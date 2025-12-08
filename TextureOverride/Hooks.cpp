#include "TextureOverride/Hooks.hpp"
#include "TextureOverride/Loading.hpp"

namespace fs = std::filesystem;


namespace TextureOverride
{

	t_UGameEngine_Exec* UGameEngine_Exec_orig = nullptr;
	DWORD UGameEngine_Exec_hook(UGameEngine* const Context, WCHAR const* const Command, void* const Archive)
	{
		if (std::wstring_view const CommandView{ Command };
			CommandView.starts_with(L"to.") || CommandView.starts_with(L"TO."))
		{
			FString const CommandCopy{ Command };
			if (CommandCopy.Contains(L"to.disable"))
			{
				g_enableLoadingManifest = false;
				LEASI_WARN(L"texture override disabled via console command");
				LEASI_WARN(L"manifests will still be processed");
			}
			else if (CommandCopy.Contains(L"to.enable"))
			{
				g_enableLoadingManifest = true;
				LEASI_WARN(L"texture override re-enabled via console command");
			}
#ifdef _DEBUG
			else if (CommandCopy.Contains(L"to.stats"))
			{
				g_statTextureSerializeCount = std::max<int>(g_statTextureSerializeCount, 1);

				LEASI_INFO(L"(stats) UTexture2D::Serialize time added:     {:.4f} ms", g_statTextureSerializeSeconds * 1000);
				LEASI_INFO(L"(stats) UTexture2D::Serialize active count:   {}", g_statTextureSerializeCount);
				LEASI_INFO(L"(stats) ==================================================");
				LEASI_INFO(L"(stats) AVERAGE TIME ADDED:                   {:.4f} ms", g_statTextureSerializeSeconds * 1000 / g_statTextureSerializeCount);
			}
#endif
		}

		return UGameEngine_Exec_orig(Context, Command, Archive);
	}

	t_UTexture2D_Serialize* UTexture2D_Serialize_orig = nullptr;
	void UTexture2D_Serialize_hook(UTexture2D* const Context, void* const Archive)
	{
		FString const& TextureFullName = GetTextureFullName(Context);
		// LEASI_DEBUG(L"UTexture2D::Serialize: {}", *TextureFullName);

		(*UTexture2D_Serialize_orig)(Context, Archive);

#if defined(SDK_TARGET_LE2) || defined(SDK_TARGET_LE3)
		if (!bHasPerformedDLCTFCRegistration && !(Context->TFCFileGuid.A == Context->TFCFileGuid.B == Context->TFCFileGuid.C == Context->TFCFileGuid.D == 0))
		{
			// Register DLC TFCs on first texture serialize to ensure they are available
			// when UpdateResource() is called as DLC mount comes too late.	
			// LE1 will always have the TFCs registered by autoload so this is not necessary for that.
			if (!bHasPerformedDLCTFCRegistration) {
				bHasPerformedDLCTFCRegistration = true;
				RegisterDLCTFCs();
			}
		}
#endif

		if (g_enableLoadingManifest)
		{
#ifdef _DEBUG
			ScopedTimer Timer{};
#endif

			for (ManifestLoaderPointer const& Manifest : g_loadedManifests)
			{
				CTextureEntry const* const Entry = Manifest->FindEntry(TextureFullName);
				if (Entry == nullptr) continue;
				LEASI_INFO(L"UTexture2D::Serialize: replacing {}", *TextureFullName);
				UpdateTextureFromManifest(Context, *Manifest, *Entry);
				return;  // apply only the highest-priority manifest mount
			}

#ifdef _DEBUG
			g_statTextureSerializeCount++;
			g_statTextureSerializeSeconds += Timer.GetSeconds();
#endif
		}

		// Clown mode
		// Context->InternalFormatLODBias = 12;
	}

	t_OodleDecompress* OodleDecompress = nullptr;

#if defined(SDK_TARGET_LE2) || defined(SDK_TARGET_LE3)
	tRegisterTFC* RegisterTFC = nullptr;

	tInternalFindFiles* InternalFindFiles_orig = nullptr;
	tInternalFindFiles* InternalFindFiles = nullptr;
	void InternalFindFiles_hook(void* self, TArray<FString>* result, const wchar_t* searchPath, bool findFiles, bool findFolders, unsigned int flags) {

	// Left here in case you ever need to turn it on to inspect things.
#if _DEBUG && FALSE
		if (findFiles && findFolders) 
			LEASI_INFO(L"InternalFindFiles [FILES AND FOLDERS] {}: {}", flags, searchPath);
		}
		else if (findFiles) {
			LEASI_INFO(L"InternalFindFiles: [FILES] {}: {}", flags, searchPath);
		}
		else if (findFolders) {
			LEASI_INFO(L"InternalFindFiles: [FOLDERS] {}: {}", flags, searchPath);
		}
		else {
			LEASI_INFO(L"InternalFindFiles: [???] {}: {}", flags, searchPath);
		}
#endif
		InternalFindFiles_orig(self, result, searchPath, findFiles, findFolders, flags);
	// Left here if you ever need to find things later.
#if _DEBUG && FALSE
		if (findFolders) {
			for (auto& entry : *result) {
				fs::path const FilePath{ entry.Chars() };
				LEASI_INFO(L"  {}", FilePath.c_str());
			}
		}
#endif
	}

	bool bHasPerformedDLCTFCRegistration = false;
	void** GFileManager = nullptr;

	void RegisterDLCTFCs() {
		// List DLC_MOD_ folders
		const wchar_t* searchPath = L"..\\..\\BIOGame\\DLC\\DLC_MOD_*";
		std::filesystem::path dlcPath = L"..\\..\\BIOGame\\DLC";

		TArray<FString> dlcFolders{};
		InternalFindFiles(*GFileManager, &dlcFolders, searchPath, false, true, 2);
		for (unsigned int i = 0; i < dlcFolders.Count(); i++) {
			// Now enumerate *.tfc files in the DLC folders.
			auto dlcName = dlcFolders.GetData()[i];

			// We check existence to prevent crash if subfolder is not set up properly
			auto dlcCookedPath = dlcPath / dlcName.Chars() / L"CookedPCConsole";
			if (std::filesystem::exists(dlcCookedPath)) {
				// Find *.tfc files in the cookedpcconsole folder
				auto tfcSearchPattern = dlcCookedPath / L"*.tfc";
				LEASI_TRACE(L"Searching in {}", dlcCookedPath.c_str());
				TArray<FString> tfcFiles{};
				InternalFindFiles(*GFileManager, &tfcFiles, tfcSearchPattern.c_str(), true, false, 2);
				for (unsigned int k = 0; k < tfcFiles.Count(); k++) {
					// Register each TFC using full path
					auto fullTfcPath = dlcCookedPath / tfcFiles.GetData()[k].Chars();
					LEASI_INFO(L"Registering DLC mod TFC: {}", fullTfcPath.c_str());
					FString tfcPath(fullTfcPath.c_str());
					RegisterTFC(&tfcPath);
				}
			}
		}
	}

#endif
}
