#include <fstream>
#include <string>
#include "Hooks.hpp"
#include "Common/Objects.hpp"

namespace ConsoleExtension
{
	// ! Constants
	// ========================================

	constexpr auto savedCamsFileName = "savedCams";
	constexpr auto savedCamsLength = 100;

	// ! Global variables
	// ========================================

	FTPOV cachedPOV;
	FTPOV povToLoad;
	bool shouldSetCamPOV = false;
	FTPOV savedPOVs[savedCamsLength];

	// ! Helper functions
	// ========================================

	void writeCamArray(const std::string& file_name, FTPOV* data)
	{
		std::ofstream out;
		out.open(file_name, std::ios::binary);
		out.write(reinterpret_cast<char*>(data), sizeof(FTPOV) * savedCamsLength);
		out.close();
	}

	void readCamArray(const std::string& file_name, FTPOV* data)
	{
		std::ifstream in;
		in.open(file_name, std::ios::binary);
		if (in.good())
		{
			in.read(reinterpret_cast<char*>(data), savedCamsLength * sizeof(FTPOV));
		}
		in.close();
	}

	bool IsCmd(wchar_t** cmd, const wchar_t* check)
	{
		const size_t len = wcslen(check);
		if (_wcsnicmp(*cmd, check, len) == 0)
		{
			*cmd += len;
			return true;
		}
		return false;
	}

	void CauseRemoteEvent(ABioPlayerController* player, SFXName eventName)
	{
		if (player)
		{
			player->CauseEvent(eventName);
		}
	}

	// ! UEngine::Exec hook
	// ========================================

	t_UEngine_Exec* UEngine_Exec_orig = nullptr;
	unsigned UEngine_Exec_hook(UEngine* Context, wchar_t* cmd, void* unk)
	{
		const auto seps = L" \t";

		// Try the original handler first
		if (UEngine_Exec_orig(Context, cmd, unk))
		{
			return TRUE;
		}

		// Handle our custom commands
		if (IsCmd(&cmd, L"savecam"))
		{
			wchar_t* context = nullptr;
			const wchar_t* const token = wcstok_s(cmd, seps, &context);
			if (token != nullptr)
			{
				if (const int i = _wtoi(token); i >= 0 && i < savedCamsLength)
				{
					savedPOVs[i] = cachedPOV;
					writeCamArray(savedCamsFileName, savedPOVs);
					return TRUE;
				}
			}
		}
		else if (IsCmd(&cmd, L"loadcam"))
		{
			wchar_t* context = nullptr;
			const wchar_t* const token = wcstok_s(cmd, seps, &context);
			if (token != nullptr)
			{
				if (const int i = _wtoi(token); i >= 0 && i < savedCamsLength)
				{
					readCamArray(savedCamsFileName, savedPOVs);
					povToLoad = savedPOVs[i];
					shouldSetCamPOV = true;
					return TRUE;
				}
			}
		}
		else if (IsCmd(&cmd, L"remoteevent") || IsCmd(&cmd, L"re"))
		{
			wchar_t* context = nullptr;
			const wchar_t* const token = wcstok_s(cmd, seps, &context);
			if (token != nullptr)
			{
				if (const auto player = ::Common::FindFirstObject<ABioPlayerController>())
				{
					SFXName eventName{ token, 0 };
					CauseRemoteEvent(player, eventName);
					return TRUE;
				}
			}
		}
#ifdef SDK_TARGET_LE3
		//LE1 and LE2 have these commands built in
		else if (IsCmd(&cmd, L"streamlevelin"))
		{
			wchar_t* context = nullptr;
			const wchar_t* const token = wcstok_s(cmd, seps, &context);
			if (token != nullptr)
			{
				if (const auto cheatMan = ::Common::FindFirstObject<UBioCheatManager>())
				{
					SFXName levelName{ token, 0 };
					cheatMan->StreamLevelIn(levelName);
					return TRUE;
				}
			}
		}
		else if (IsCmd(&cmd, L"streamlevelout"))
		{
			wchar_t* context = nullptr;
			const wchar_t* const token = wcstok_s(cmd, seps, &context);
			if (token != nullptr)
			{
				if (const auto cheatMan = ::Common::FindFirstObject<UBioCheatManager>())
				{
					SFXName levelName{ token, 0 };
					cheatMan->StreamLevelOut(levelName);
					return TRUE;
				}
			}
		}
		else if (IsCmd(&cmd, L"onlyloadlevel"))
		{
			wchar_t* context = nullptr;
			const wchar_t* const token = wcstok_s(cmd, seps, &context);
			if (token != nullptr)
			{
				if (const auto cheatMan = ::Common::FindFirstObject<UBioCheatManager>())
				{
					SFXName levelName{ token, 0 };
					cheatMan->OnlyLoadLevel(levelName);
					return TRUE;
				}
			}
		}
#endif

		return FALSE;
	}

	// ! UObject::ProcessEvent hook
	// ========================================

#ifdef SDK_TARGET_LE3
	constexpr auto PLAYERTICK_FUNCNAME = L"Function SFXGame.BioPlayerController.PlayerTick";
#else
	constexpr auto PLAYERTICK_FUNCNAME = L"Function Engine.PlayerController.PlayerTick";
#endif

	t_UObject_ProcessEvent* UObject_ProcessEvent_orig = nullptr;
	void UObject_ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		auto funcFullName = Function->GetFullName();

		if (Context->IsA(APlayerController::StaticClass()) && funcFullName.Equals(PLAYERTICK_FUNCNAME))
		{
			const auto playerController = static_cast<APlayerController*>(Context);
			if (playerController->PlayerCamera)
			{
				cachedPOV = playerController->PlayerCamera->CameraCache.POV;
			}
		}
		else if (shouldSetCamPOV && funcFullName.Equals(L"Function Engine.Camera.UpdateCamera"))
		{
			if (const auto camera = static_cast<ACamera*>(Context))
			{
				if (camera->PCOwner && camera->PCOwner->IsA(AActor::StaticClass()))
				{
					shouldSetCamPOV = false;
					const auto actor = static_cast<AActor*>(camera->PCOwner);
					actor->LOCATION = povToLoad.LOCATION;
					actor->Rotation = povToLoad.Rotation;
				}
			}
		}

		UObject_ProcessEvent_orig(Context, Function, Parms, Result);
	}

#ifdef SDK_TARGET_LE3
	// ! StaticConstructObject for LE3
	// ========================================

	t_StaticConstructObject* StaticConstructObject = nullptr;
#endif
}
