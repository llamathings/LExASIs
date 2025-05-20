#![allow(dead_code)]

use std::sync::Arc;

use anyhow::{anyhow, Context};
use axum::{extract::{ws::WebSocket, State, WebSocketUpgrade}, http::StatusCode, response::Html, routing::{any, delete, get, post, put}, Router};
use serde::Serialize;
use tokio::{net::TcpListener, sync::{broadcast::{self, Sender}, RwLock}};

fn main() {
    let (sender, _) = broadcast::channel::<WebNotif>(1);

    tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build().expect("failed to build tokio runtime")
        .block_on(async {
            let app = Router::new()
                .route("/", get(async || { Html(include_str!("./index.html")) }))
                .route("/hello", get(async || { "Hello, world!!!" }))
                .route("/conversation", post(async |State(state): State<AppState>| {
                    match state.start_conversation().await {
                        Ok(()) => (StatusCode::NO_CONTENT, "".to_string()),
                        Err(err) => (StatusCode::INTERNAL_SERVER_ERROR, err.to_string()),
                    }
                }))
                .route("/conversation", delete(async |State(state): State<AppState>| {
                    match state.end_conversation().await {
                        Ok(()) => (StatusCode::NO_CONTENT, "".to_string()),
                        Err(err) => (StatusCode::INTERNAL_SERVER_ERROR, err.to_string()),
                    }
                }))
                .route("/conversation/replies", put(async |State(state): State<AppState>, body: String| {
                    match state.update_replies(&body).await {
                        Ok(()) => (StatusCode::NO_CONTENT, "".to_string()),
                        Err(err) => (StatusCode::INTERNAL_SERVER_ERROR, err.to_string()),
                    }
                }))
                .route("/keybinds", put(async |State(state): State<AppState>, body: String| {
                    match state.update_keybinds(&body).await {
                        Ok(()) => (StatusCode::NO_CONTENT, "".to_string()),
                        Err(err) => (StatusCode::INTERNAL_SERVER_ERROR, err.to_string()),
                    }
                }))
                .route("/socket", any(async |ws: WebSocketUpgrade, State(state): State<AppState>| {
                    async fn handle_socket(mut socket: WebSocket, state: AppState) {
                        // Sync prior state for new clients...
                        {
                            let guard = state.convo.read().await;
                            if let Some(ref convo) = *guard {
                                let start_notif = WebNotif::Conversation { start: true };
                                let start_notif_str = serde_json::to_string(&start_notif).unwrap();
                                if socket.send(start_notif_str.into()).await.is_err() { return; }

                                let replies_notif = WebNotif::UpdateReplies { replies: convo.replies.to_vec() };
                                let replies_notif_str = serde_json::to_string(&replies_notif).unwrap();
                                if socket.send(replies_notif_str.into()).await.is_err() { return; }

                                let keybinds_notif = WebNotif::SetKeybinds { keybinds: state.keybinds.read().await.to_vec() };
                                let keybinds_notif_str = serde_json::to_string(&keybinds_notif).unwrap();
                                if socket.send(keybinds_notif_str.into()).await.is_err() { return; }
                            }
                        }

                        let mut receiver = state.sender.subscribe();
                        tokio::spawn(async move {
                            while let Ok(notif) = receiver.recv().await {
                                let string = serde_json::to_string(&notif).unwrap();
                                if socket.send(string.into()).await.is_err() { return; }
                            }
                        });
                    }

                    ws.on_upgrade(|socket: WebSocket| handle_socket(socket, state))
                }))
                .with_state(AppState::new(sender));

            let listener = TcpListener::bind("0.0.0.0:21830").await.expect("failed to listen");
            println!("listening on {}, press ctrl+c to quit...", listener.local_addr().map(|a| a.to_string()).unwrap_or_default());
            axum::serve(listener, app).await.expect("failed to serve");
        })
}

/// Maximum number of available replies we can handle.
const MAX_REPLIES: usize = 16;

/// Server state passed between requests.
#[derive(Clone)]
pub struct AppState {
    /// Active conversation instance.
    pub convo: Arc<RwLock<Option<Conversation>>>,
    /// Key bindings for client-side reply selection.
    pub keybinds: Arc<RwLock<[String; MAX_REPLIES]>>,

    sender: Arc<Sender<WebNotif>>,
}

impl AppState {
    pub fn new(sender: Sender<WebNotif>) -> Self {
        Self {
            convo: Arc::new(RwLock::new(None)),
            keybinds: Arc::new(RwLock::new(Default::default())),
            sender: Arc::new(sender),
        }
    }

    /// Initializes a new conversation.
    pub async fn start_conversation(&self) -> anyhow::Result<()> {
        let mut convo = self.convo.write().await;
        match *convo {
            Some(_) => Err(anyhow!("conversation already started"))?,
            None => {
                *convo = Some(Conversation::new());
                self.sender.send(WebNotif::Conversation { start: true })?;
                Ok(())
            },
        }
    }

    /// Terminates current conversation.
    pub async fn end_conversation(&self) -> anyhow::Result<()> {
        let mut convo = self.convo.write().await;
        match *convo {
            Some(_) => {
                *convo = None;
                self.sender.send(WebNotif::Conversation { start: false })?;
                Ok(())
            },
            None => Err(anyhow!("conversation not started"))?,
        }
    }

    /// Updates current conversation's displayed replies.
    pub async fn update_replies(&self, client_string: &str) -> anyhow::Result<()> {
        let mut convo = self.convo.write().await;
        match *convo {
            Some(ref mut convo) => {
                convo.clear();
                convo.parse(client_string)?;
                self.sender.send(WebNotif::UpdateReplies { replies: convo.replies.to_vec() })?;
                Ok(())
            },
            None => Err(anyhow!("conversation not started"))?,
        }
    }

    /// Updates displayed key binding configuration.
    pub async fn update_keybinds(&self, client_string: &str) -> anyhow::Result<()> {
        let tokens: Vec<String> = client_string.split(';').map(|s| s.to_string()).collect();
        let names: &[String; MAX_REPLIES] = tokens.as_slice().try_into().context("invalid keybind count")?;

        self.keybinds.write().await.clone_from(names);
        self.sender.send(WebNotif::SetKeybinds { keybinds: names.to_vec() })?;

        Ok(())
    }
}

/// Active conversation instance.
#[derive(Default)]
pub struct Conversation {
    pub replies: [ConversationReply; MAX_REPLIES],
}

impl Conversation {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn clear(&mut self) {
        self.replies.iter_mut()
            .for_each(|reply| reply.clear());
    }

    pub fn parse(&mut self, source: &str) -> anyhow::Result<()> {
        let lines = source.lines().map(str::trim).collect::<Vec<_>>();
        if let Some((&head, tail)) = lines.split_first() {
            let count: usize = head.parse().map_err(|_| anyhow!("invalid count line"))?;
            if count >= MAX_REPLIES { Err(anyhow!("reply count too high: {}", count))?; }

            let mut lines = tail.iter();
            for i in 0..count {
                let line_separator = *lines.next().ok_or_else(|| anyhow!("failed to pop separator line for reply {}", i))?;
                let line_index = *lines.next().ok_or_else(|| anyhow!("failed to pop index line for reply {}", i))?;
                let line_style = *lines.next().ok_or_else(|| anyhow!("failed to pop style line for reply {}", i))?;
                let line_paraphrase = *lines.next().ok_or_else(|| anyhow!("failed to pop paraphrase line for reply {}", i))?;
                let line_text = *lines.next().ok_or_else(|| anyhow!("failed to pop text line for reply {}", i))?;

                if !line_separator.is_empty() { Err(anyhow!("invalid separator line for reply {}", i))?; }

                let index: usize = line_index.parse().map_err(|_| anyhow!("invalid index line for reply {}", i))?;
                if index >= MAX_REPLIES { Err(anyhow!("reply index too high ({}) for reply {}", count, i))?; }

                self.replies[index] = ConversationReply {
                    id: index,
                    style: line_style.to_string(),
                    paraphrase: line_paraphrase.to_string(),
                    text: line_text.to_string(),
                };
            }

            Ok(())
        } else {
            Err(anyhow!("too few lines in reply manifest"))
        }
    }
}

/// Displayed conversation reply.
#[derive(Clone, Debug, Default, Serialize)]
pub struct ConversationReply {
    pub id: usize,
    pub style: String,
    pub paraphrase: String,
    pub text: String,
}

impl ConversationReply {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn clear(&mut self) {
        self.id = 0;
        self.style.clear();
        self.paraphrase.clear();
        self.text.clear();
    }
}


/// Schema for notifications sent to clients over WebSockets connections.
#[derive(Clone, Debug, Serialize)]
pub enum WebNotif {
    Conversation { start: bool, },
    UpdateReplies {
        replies: Vec<ConversationReply>,
    },
    SetKeybinds {
        keybinds: Vec<String>,
    },
}
