#![allow(dead_code)]

use std::{fs::File, net::SocketAddr};
use std::sync::Arc;

use anyhow::{anyhow, Context};
use axum::extract::ConnectInfo;
use axum::response::{IntoResponse, Response};
use axum::{extract::{ws::WebSocket, State, WebSocketUpgrade}, http::StatusCode, response::Html, routing::{any, delete, get, post, put}, Router};
use local_ip_address::local_ip;
use serde::Serialize;
use tokio::{net::TcpListener, sync::{broadcast::{self, Sender}, RwLock}};
use tracing::level_filters::LevelFilter;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};


/// Maximum number of available replies we can handle.
const MAX_REPLIES: usize = 16;


/// Shortens the parameters of HTTP handlers.
macro_rules! handle {
    ($state:ident, $body:ident, $do:block) => {
        handle! { $state, _addr, $body, $do }
    };
    ($state:ident, $addr:ident, $body:ident, $do:block) => {
        #[allow(unused_variables)]
        async |State($state): State<AppState>, ConnectInfo($addr): ConnectInfo<SocketAddr>, $body: String| -> HandlerResult {
            $do
        }
    };
}


fn main() {
    init_logger().expect("failed to initialize logger");

    // Keep a receiver copy around to avoid "channel closed" errors.
    let (sender, _receiver) = broadcast::channel::<WebNotif>(1);

    tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
            .inspect_err(|err| tracing::error!("failed to build tokio runtime: {}", err))
            .unwrap()
        .block_on(async {
            let app = Router::new()
                .route("/", get(async || { Html(include_str!("./index.html")) }))
                .route("/hello", get(async || { "Hello, world!!!" }))
                .route("/conversation", post(handle! { state, addr, body, {
                    tracing::info!(?addr, "starting new conversation");
                    state.start_conversation().await.inspect_err(|e| tracing::error!(?addr, "{}", e))?;
                    Ok((StatusCode::NO_CONTENT, "").into_response())
                }}))
                .route("/conversation", delete(handle! { state, addr, body, {
                    tracing::info!(?addr, "ending current conversation");
                    state.end_conversation().await.inspect_err(|e| tracing::error!(?addr, "{}", e))?;
                    Ok((StatusCode::NO_CONTENT, "").into_response())
                }}))
                .route("/conversation/replies/queue", post(handle! { state, addr, body, {
                    tracing::info!(?addr, "queueing new reply for current conversation");
                    state.queue_reply(&body).await.inspect_err(|e| tracing::error!(?addr, "{}", e))?;
                    Ok((StatusCode::NO_CONTENT, "").into_response())
                }}))
                .route("/conversation/replies", put(handle! { state, addr, body, {
                    tracing::info!(?addr, "syncing replies for current conversation");
                    state.update_replies(&body).await.inspect_err(|e| tracing::error!(?addr, "{}", e))?;
                    Ok((StatusCode::NO_CONTENT, "").into_response())
                }}))
                .route("/keybinds", put(handle! { state, addr, body, {
                    tracing::info!(?addr, "syncing keybinds");
                    state.update_keybinds(&body).await.inspect_err(|e| tracing::error!(?addr, "{}", e))?;
                    Ok((StatusCode::NO_CONTENT, "").into_response())
                }}))
                .route("/socket", any(async |ws: WebSocketUpgrade, State(state): State<AppState>, ConnectInfo(addr): ConnectInfo<SocketAddr>| {
                    async fn handle_socket(mut socket: WebSocket, state: AppState, addr: SocketAddr) {
                        tracing::info!(?addr, "handling new websocket connection");

                        // Sync keybinds for new clients.
                        let keybinds_notif = WebNotif::SetKeybinds { keybinds: state.keybinds.read().await.to_vec() };
                        let keybinds_notif_str = serde_json::to_string(&keybinds_notif).unwrap();
                        if socket.send(keybinds_notif_str.into()).await.is_err() { return; }

                        // Sync prior conversation state for new clients.
                        {
                            let guard = state.convo.read().await;
                            if let Some(ref convo) = *guard {
                                let start_notif = WebNotif::Conversation { start: true };
                                let start_notif_str = serde_json::to_string(&start_notif).unwrap();
                                if socket.send(start_notif_str.into()).await.is_err() { return; }

                                let replies_notif = WebNotif::UpdateReplies { replies: convo.replies.to_vec() };
                                let replies_notif_str = serde_json::to_string(&replies_notif).unwrap();
                                if socket.send(replies_notif_str.into()).await.is_err() { return; }
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

                    ws.on_upgrade(move |socket: WebSocket| handle_socket(socket, state, addr))
                }))
                .with_state(AppState::new(sender))
                .into_make_service_with_connect_info::<SocketAddr>();

            let listener = TcpListener::bind("0.0.0.0:21830").await
                .inspect_err(|err| tracing::error!("failed to bind tcp listener: {}", err))
                .unwrap();

            tracing::info!("listening on {}, press ctrl+c to quit...", listener.local_addr().map(|a| a.to_string()).unwrap_or_default());
            if let (Ok(ip), Ok(port)) = (local_ip(), listener.local_addr().map(|a| a.port())) {
                tracing::info!("you can open http://{}:{} in your browser", ip, port);
            }

            axum::serve(listener, app).await
                .inspect_err(|err| tracing::error!("failed to serve axum service: {}", err))
                .unwrap();
        })
}

/// Initializes the global logger.
fn init_logger() -> anyhow::Result<()> {
    let file = tracing_subscriber::fmt::layer()
        .compact()
        .with_ansi(false)
        .with_file(false)
        .with_line_number(false)
        .with_target(false)
        .with_writer(File::create("./convosniffer.server.log")?);
    let console = tracing_subscriber::fmt::layer()
        .pretty();

    tracing_subscriber::registry()
        .with(LevelFilter::DEBUG)
        .with(file)
        .with(console)
        .try_init()
        .map_err(|e| e.into())
}


/// Specialization of [`Result`] for [`HandlerError`].
pub type HandlerResult = Result<Response, HandlerError>;

/// Type for handler errors.
#[derive(Debug)]
pub struct HandlerError(pub anyhow::Error);
impl<E: Into<anyhow::Error>> From<E> for HandlerError {
    fn from(value: E) -> Self {
        Self(value.into())
    }
}
impl IntoResponse for HandlerError {
    fn into_response(self) -> Response {
        (StatusCode::INTERNAL_SERVER_ERROR, self.0.to_string()).into_response()
    }
}


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

        if convo.is_some() {
            // Reset the conversation instead of hard error if out of sync...
            tracing::warn!("conversation already started, resetting...");
        }

        *convo = Some(Conversation::new());
        drop(convo);

        self.sender.send(WebNotif::Conversation { start: true })
            .map_err(|e| e.into())
            .map(|_| ())
    }

    /// Terminates current conversation.
    pub async fn end_conversation(&self) -> anyhow::Result<()> {
        let mut convo = self.convo.write().await;

        if convo.is_none() {
            // Reset the conversation instead of hard error if out of sync...
            tracing::warn!("conversation not started, resetting...");
        }

        *convo = None;
        drop(convo);

        self.sender.send(WebNotif::Conversation { start: false })
            .map_err(|e| e.into())
            .map(|_| ())
    }

    /// Queues a reply in the current conversation.
    pub async fn queue_reply(&self, client_string: &str) -> anyhow::Result<()> {
        let index: i32 = client_string.trim().parse().context("invalid index integer")?;
        if index < 0 || index >= MAX_REPLIES as i32 { Err(anyhow!("index {} out of bounds", index))?; }

        let mut convo = self.convo.write().await;
        match &mut *convo {
            Some(convo) => {
                convo.replies[index as usize].queued = true;
                self.sender.send(WebNotif::QueueReply { index })?;
                Ok(())
            },
            None => Err(anyhow!("conversation not started"))?,
        }
    }

    /// Updates current conversation's displayed replies.
    pub async fn update_replies(&self, client_string: &str) -> anyhow::Result<()> {
        let mut send_error = None;

        let mut convo = self.convo.write().await;
        let convo_ref = convo.get_or_insert_with(|| {
            // Implicitly create the conversation instead of hard error if out of sync...
            if let Err(error) = self.sender.send(WebNotif::Conversation { start: true }) {
                send_error = Some(error);
            }
            Conversation::new()
        });

        // Propagate error from initialization closure.
        if let Some(error) = send_error {
            Err(error)?;
        }

        convo_ref.clear();
        convo_ref.parse(client_string)?;
        self.sender.send(WebNotif::UpdateReplies { replies: convo_ref.replies.to_vec() })
            .map_err(|e| e.into())
            .map(|_| ())
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
                let line_category = *lines.next().ok_or_else(|| anyhow!("failed to pop category line for reply {}", i))?;
                let line_paraphrase = *lines.next().ok_or_else(|| anyhow!("failed to pop paraphrase line for reply {}", i))?;
                let line_text = *lines.next().ok_or_else(|| anyhow!("failed to pop text line for reply {}", i))?;

                if !line_separator.is_empty() { Err(anyhow!("invalid separator line for reply {}", i))?; }

                let index: usize = line_index.parse().map_err(|_| anyhow!("invalid index line for reply {}", i))?;
                if index >= MAX_REPLIES { Err(anyhow!("reply index too high ({}) for reply {}", count, i))?; }

                self.replies[index] = ConversationReply {
                    id: index,
                    style: line_style.to_string(),
                    category: line_category.to_string(),
                    paraphrase: line_paraphrase.to_string(),
                    text: line_text.to_string(),
                    queued: false,
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
    pub category: String,
    pub paraphrase: String,
    pub text: String,
    pub queued: bool,
}

impl ConversationReply {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn clear(&mut self) {
        self.id = 0;
        self.style.clear();
        self.category.clear();
        self.paraphrase.clear();
        self.text.clear();
        self.queued = false;
    }
}


/// Schema for notifications sent to clients over WebSockets connections.
#[derive(Clone, Debug, Serialize)]
pub enum WebNotif {
    Conversation { start: bool, },
    UpdateReplies {
        replies: Vec<ConversationReply>,
    },
    QueueReply {
        index: i32,
    },
    SetKeybinds {
        keybinds: Vec<String>,
    },
}
