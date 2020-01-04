// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/resource/resource_bundle.h"

#include <stdint.h>

#include <limits>
#include <utility>
#include <vector>

#include "base/big_endian.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/layout.h"
#include "ui/base/resource/data_pack.h"
#include "ui/base/ui_base_paths.h"
#include "ui/strings/grit/app_locale_settings.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#include "ui/base/resource/resource_bundle_android.h"
#endif

namespace ui {

namespace {

// PNG-related constants.
const unsigned char kPngMagic[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
const size_t kPngChunkMetadataSize = 12;  // length, type, crc32
const unsigned char kPngScaleChunkType[4] = { 'c', 's', 'C', 'l' };
const unsigned char kPngDataChunkType[4] = { 'I', 'D', 'A', 'T' };

#if !defined(OS_MACOSX)
const char kPakFileExtension[] = ".pak";
#endif

ResourceBundle* g_shared_instance_ = NULL;

base::FilePath GetResourcesPakFilePath(const std::string& pak_name) {
  base::FilePath path;
  if (base::PathService::Get(base::DIR_MODULE, &path))
    return path.AppendASCII(pak_name.c_str());

  // Return just the name of the pak file.
#if defined(OS_WIN)
  return base::FilePath(base::ASCIIToUTF16(pak_name));
#else
  return base::FilePath(pak_name.c_str());
#endif  // OS_WIN
}

}  // namespace

// static
std::string ResourceBundle::InitSharedInstanceWithLocale(
    const std::string& pref_locale,
    Delegate* delegate,
    LoadResources load_resources) {
  InitSharedInstance(delegate);
  if (load_resources == LOAD_COMMON_RESOURCES)
    g_shared_instance_->LoadCommonResources();
  std::string result = g_shared_instance_->LoadLocaleResources(pref_locale);
  return result;
}

// static
void ResourceBundle::InitSharedInstanceWithPakFileRegion(
    base::File pak_file,
    const base::MemoryMappedFile::Region& region) {
  InitSharedInstance(NULL);
  auto data_pack = std::make_unique<DataPack>(SCALE_FACTOR_100P);
  if (!data_pack->LoadFromFileRegion(std::move(pak_file), region)) {
    LOG(WARNING) << "failed to load pak file";
    NOTREACHED();
    return;
  }
  g_shared_instance_->locale_resources_data_ = std::move(data_pack);
}

// static
void ResourceBundle::InitSharedInstanceWithPakPath(const base::FilePath& path) {
  InitSharedInstance(NULL);
  g_shared_instance_->LoadTestResources(path, path);

}

// static
void ResourceBundle::CleanupSharedInstance() {
  delete g_shared_instance_;
  g_shared_instance_ = NULL;
}

// static
bool ResourceBundle::HasSharedInstance() {
  return g_shared_instance_ != NULL;
}

// static
ResourceBundle& ResourceBundle::GetSharedInstance() {
  // Must call InitSharedInstance before this function.
  CHECK(g_shared_instance_ != NULL);
  return *g_shared_instance_;
}

void ResourceBundle::LoadSecondaryLocaleDataWithPakFileRegion(
    base::File pak_file,
    const base::MemoryMappedFile::Region& region) {
  auto data_pack = std::make_unique<DataPack>(SCALE_FACTOR_100P);
  if (!data_pack->LoadFromFileRegion(std::move(pak_file), region)) {
    LOG(WARNING) << "failed to load secondary pak file";
    NOTREACHED();
    return;
  }
  secondary_locale_resources_data_ = std::move(data_pack);
}

#if !defined(OS_ANDROID)
bool ResourceBundle::LocaleDataPakExists(const std::string& locale) {
  return !GetLocaleFilePath(locale, true).empty();
}
#endif  // !defined(OS_ANDROID)

void ResourceBundle::AddDataPack(std::unique_ptr<DataPack> data_pack) {
#if DCHECK_IS_ON()
  data_pack->CheckForDuplicateResources(data_packs_);
#endif

  if (GetScaleForScaleFactor(data_pack->GetScaleFactor()) >
      GetScaleForScaleFactor(max_scale_factor_))
    max_scale_factor_ = data_pack->GetScaleFactor();

  data_packs_.push_back(std::move(data_pack));
}

void ResourceBundle::AddDataPackFromPath(const base::FilePath& path,
                                         ScaleFactor scale_factor) {
  AddDataPackFromPathInternal(path, scale_factor, false);
}

void ResourceBundle::AddOptionalDataPackFromPath(const base::FilePath& path,
                                                 ScaleFactor scale_factor) {
  AddDataPackFromPathInternal(path, scale_factor, true);
}

void ResourceBundle::AddDataPackFromFile(base::File file,
                                         ScaleFactor scale_factor) {
  AddDataPackFromFileRegion(std::move(file),
                            base::MemoryMappedFile::Region::kWholeFile,
                            scale_factor);
}

void ResourceBundle::AddDataPackFromBuffer(base::StringPiece buffer,
                                           ScaleFactor scale_factor) {
  std::unique_ptr<DataPack> data_pack(new DataPack(scale_factor));
  if (data_pack->LoadFromBuffer(buffer)) {
    AddDataPack(std::move(data_pack));
  } else {
    LOG(ERROR) << "Failed to load data pack from buffer";
  }
}

void ResourceBundle::AddDataPackFromFileRegion(
    base::File file,
    const base::MemoryMappedFile::Region& region,
    ScaleFactor scale_factor) {
  std::unique_ptr<DataPack> data_pack(new DataPack(scale_factor));
  if (data_pack->LoadFromFileRegion(std::move(file), region)) {
    AddDataPack(std::move(data_pack));
  } else {
    LOG(ERROR) << "Failed to load data pack from file."
               << "\nSome features may not be available.";
  }
}

#if !defined(OS_MACOSX)
base::FilePath ResourceBundle::GetLocaleFilePath(const std::string& app_locale,
                                                 bool test_file_exists) {
  if (app_locale.empty())
    return base::FilePath();

  base::FilePath locale_file_path;

  base::PathService::Get(ui::DIR_LOCALES, &locale_file_path);

  if (!locale_file_path.empty()) {
#if defined(OS_ANDROID)
    if (locale_file_path.value().find("chromium_tests") == std::string::npos) {
      std::string extracted_file_suffix =
          base::android::BuildInfo::GetInstance()->extracted_file_suffix();
      locale_file_path = locale_file_path.AppendASCII(
          app_locale + kPakFileExtension + extracted_file_suffix);
    } else {
      // TODO(agrieve): Update tests to not side-load pak files and remove
      //     this special-case. https://crbug.com/691719
      locale_file_path =
          locale_file_path.AppendASCII(app_locale + kPakFileExtension);
    }
#else
    locale_file_path =
        locale_file_path.AppendASCII(app_locale + kPakFileExtension);
#endif
  }

  if (delegate_) {
    locale_file_path =
        delegate_->GetPathForLocalePack(locale_file_path, app_locale);
  }

  // Don't try to load empty values or values that are not absolute paths.
  if (locale_file_path.empty() || !locale_file_path.IsAbsolute())
    return base::FilePath();

  if (test_file_exists && !base::PathExists(locale_file_path))
    return base::FilePath();

  return locale_file_path;
}
#endif

#if !defined(OS_ANDROID)
std::string ResourceBundle::LoadLocaleResources(
    const std::string& pref_locale) {
  DCHECK(!locale_resources_data_.get()) << "locale.pak already loaded";
  std::string app_locale = l10n_util::GetApplicationLocale(pref_locale);
  base::FilePath locale_file_path = GetOverriddenPakPath();
  if (locale_file_path.empty())
    locale_file_path = GetLocaleFilePath(app_locale, true);

  if (locale_file_path.empty()) {
    // It's possible that there is no locale.pak.
    LOG(WARNING) << "locale_file_path.empty() for locale " << app_locale;
    return std::string();
  }

  std::unique_ptr<DataPack> data_pack(new DataPack(SCALE_FACTOR_100P));
  if (!data_pack->LoadFromPath(locale_file_path)) {
    LOG(ERROR) << "failed to load locale.pak";
    NOTREACHED();
    return std::string();
  }

  locale_resources_data_ = std::move(data_pack);
  return app_locale;
}
#endif  // defined(OS_ANDROID)

void ResourceBundle::LoadTestResources(const base::FilePath& path,
                                       const base::FilePath& locale_path) {
  is_test_resources_ = true;
  DCHECK(!ui::GetSupportedScaleFactors().empty());
  const ScaleFactor scale_factor(ui::GetSupportedScaleFactors()[0]);
  // Use the given resource pak for both common and localized resources.
  std::unique_ptr<DataPack> data_pack(new DataPack(scale_factor));
  if (!path.empty() && data_pack->LoadFromPath(path))
    AddDataPack(std::move(data_pack));

  data_pack.reset(new DataPack(ui::SCALE_FACTOR_NONE));
  if (!locale_path.empty() && data_pack->LoadFromPath(locale_path)) {
    locale_resources_data_ = std::move(data_pack);
  } else {
    locale_resources_data_.reset(new DataPack(ui::SCALE_FACTOR_NONE));
  }
  // This is necessary to initialize ICU since we won't be calling
  // LoadLocaleResources in this case.
  l10n_util::GetApplicationLocale(std::string());
}

void ResourceBundle::UnloadLocaleResources() {
  locale_resources_data_.reset();
  secondary_locale_resources_data_.reset();
}

void ResourceBundle::OverrideLocalePakForTest(const base::FilePath& pak_path) {
  overridden_pak_path_ = pak_path;
}

void ResourceBundle::OverrideLocaleStringResource(
    int message_id,
    const base::string16& string) {
  overridden_locale_strings_[message_id] = string;
}

const base::FilePath& ResourceBundle::GetOverriddenPakPath() {
  return overridden_pak_path_;
}

base::string16 ResourceBundle::MaybeMangleLocalizedString(
    const base::string16& str) {
  if (!mangle_localized_strings_)
    return str;

  // IDS_DEFAULT_FONT_SIZE and friends are localization "strings" that are
  // actually integral constants. These should not be mangled or they become
  // impossible to parse.
  int ignored;
  if (base::StringToInt(str, &ignored))
    return str;

  // For a string S, produce [[ --- S --- ]], where the number of dashes is 1/4
  // of the number of characters in S. This makes S something around 50-75%
  // longer, except for extremely short strings, which get > 100% longer.
  base::string16 start_marker = base::UTF8ToUTF16("[[");
  base::string16 end_marker = base::UTF8ToUTF16("]]");
  base::string16 dashes = base::string16(str.size() / 4, '-');
  return base::JoinString({start_marker, dashes, str, dashes, end_marker},
                          base::UTF8ToUTF16(" "));
}

std::string ResourceBundle::ReloadLocaleResources(
    const std::string& pref_locale) {
  base::AutoLock lock_scope(*locale_resources_data_lock_);

  // Remove all overriden strings, as they will not be valid for the new locale.
  overridden_locale_strings_.clear();

  UnloadLocaleResources();
  return LoadLocaleResources(pref_locale);
}

base::RefCountedMemory* ResourceBundle::LoadDataResourceBytes(
    int resource_id) const {
  return LoadDataResourceBytesForScale(resource_id, ui::SCALE_FACTOR_NONE);
}

base::RefCountedMemory* ResourceBundle::LoadDataResourceBytesForScale(
    int resource_id,
    ScaleFactor scale_factor) const {
  base::RefCountedMemory* bytes = NULL;
  if (delegate_)
    bytes = delegate_->LoadDataResourceBytes(resource_id, scale_factor);

  if (!bytes) {
    base::StringPiece data =
        GetRawDataResourceForScale(resource_id, scale_factor);
    if (!data.empty())
      bytes = new base::RefCountedStaticMemory(data.data(), data.length());
  }

  return bytes;
}

base::StringPiece ResourceBundle::GetRawDataResource(int resource_id) const {
  return GetRawDataResourceForScale(resource_id, ui::SCALE_FACTOR_NONE);
}

base::StringPiece ResourceBundle::GetRawDataResourceForScale(
    int resource_id,
    ScaleFactor scale_factor) const {
  base::StringPiece data;
  if (delegate_ &&
      delegate_->GetRawDataResource(resource_id, scale_factor, &data))
    return data;

  if (scale_factor != ui::SCALE_FACTOR_100P) {
    for (size_t i = 0; i < data_packs_.size(); i++) {
      if (data_packs_[i]->GetScaleFactor() == scale_factor &&
          data_packs_[i]->GetStringPiece(static_cast<uint16_t>(resource_id),
                                         &data))
        return data;
    }
  }

  for (size_t i = 0; i < data_packs_.size(); i++) {
    if ((data_packs_[i]->GetScaleFactor() == ui::SCALE_FACTOR_100P ||
         data_packs_[i]->GetScaleFactor() == ui::SCALE_FACTOR_200P ||
         data_packs_[i]->GetScaleFactor() == ui::SCALE_FACTOR_300P ||
         data_packs_[i]->GetScaleFactor() == ui::SCALE_FACTOR_NONE) &&
        data_packs_[i]->GetStringPiece(static_cast<uint16_t>(resource_id),
                                       &data))
      return data;
  }

  return base::StringPiece();
}

base::string16 ResourceBundle::GetLocalizedString(int message_id) {
  base::string16 string;
  if (delegate_ && delegate_->GetLocalizedString(message_id, &string))
    return MaybeMangleLocalizedString(string);

  // Ensure that ReloadLocaleResources() doesn't drop the resources while
  // we're using them.
  base::AutoLock lock_scope(*locale_resources_data_lock_);

  IdToStringMap::const_iterator it =
      overridden_locale_strings_.find(message_id);
  if (it != overridden_locale_strings_.end())
    return MaybeMangleLocalizedString(it->second);

  // If for some reason we were unable to load the resources , return an empty
  // string (better than crashing).
  if (!locale_resources_data_.get()) {
    LOG(WARNING) << "locale resources are not loaded";
    return base::string16();
  }

  base::StringPiece data;
  ResourceHandle::TextEncodingType encoding =
      locale_resources_data_->GetTextEncodingType();
  if (!locale_resources_data_->GetStringPiece(static_cast<uint16_t>(message_id),
                                              &data)) {
    if (secondary_locale_resources_data_.get() &&
        secondary_locale_resources_data_->GetStringPiece(
            static_cast<uint16_t>(message_id), &data)) {
      // Fall back on the secondary locale pak if it exists.
      encoding = secondary_locale_resources_data_->GetTextEncodingType();
    } else {
      // Fall back on the main data pack (shouldn't be any strings here except
      // in unittests).
      data = GetRawDataResource(message_id);
      if (data.empty()) {
        LOG(WARNING) << "unable to find resource: " << message_id;
        NOTREACHED();
        return base::string16();
      }
    }
  }

  // Strings should not be loaded from a data pack that contains binary data.
  DCHECK(encoding == ResourceHandle::UTF16 || encoding == ResourceHandle::UTF8)
      << "requested localized string from binary pack file";

  // Data pack encodes strings as either UTF8 or UTF16.
  base::string16 msg;
  if (encoding == ResourceHandle::UTF16) {
    msg = base::string16(reinterpret_cast<const base::char16*>(data.data()),
                         data.length() / 2);
  } else if (encoding == ResourceHandle::UTF8) {
    msg = base::UTF8ToUTF16(data);
  }
  return MaybeMangleLocalizedString(msg);
}

base::RefCountedMemory* ResourceBundle::LoadLocalizedResourceBytes(
    int resource_id) {
  {
    base::AutoLock lock_scope(*locale_resources_data_lock_);
    base::StringPiece data;

    if (locale_resources_data_.get() &&
        locale_resources_data_->GetStringPiece(
            static_cast<uint16_t>(resource_id), &data) &&
        !data.empty()) {
      return new base::RefCountedStaticMemory(data.data(), data.length());
    }

    if (secondary_locale_resources_data_.get() &&
        secondary_locale_resources_data_->GetStringPiece(
            static_cast<uint16_t>(resource_id), &data) &&
        !data.empty()) {
      return new base::RefCountedStaticMemory(data.data(), data.length());
    }
  }
  // Release lock_scope and fall back to main data pack.
  return LoadDataResourceBytes(resource_id);
}

void ResourceBundle::ReloadFonts() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

ScaleFactor ResourceBundle::GetMaxScaleFactor() const {
#if defined(OS_WIN) || defined(OS_LINUX)
  return max_scale_factor_;
#else
  return GetSupportedScaleFactors().back();
#endif
}

bool ResourceBundle::IsScaleFactorSupported(ScaleFactor scale_factor) {
  const std::vector<ScaleFactor>& supported_scale_factors =
      ui::GetSupportedScaleFactors();
  return base::ContainsValue(supported_scale_factors, scale_factor);
}

ResourceBundle::ResourceBundle(Delegate* delegate)
    : delegate_(delegate),
      locale_resources_data_lock_(new base::Lock),
      max_scale_factor_(SCALE_FACTOR_100P) {
  mangle_localized_strings_ = base::CommandLine::ForCurrentProcess()->HasSwitch(
      "mangle-localized-strings");
}

ResourceBundle::~ResourceBundle() {
  UnloadLocaleResources();
}

// static
void ResourceBundle::InitSharedInstance(Delegate* delegate) {
  DCHECK(g_shared_instance_ == NULL) << "ResourceBundle initialized twice";
  g_shared_instance_ = new ResourceBundle(delegate);
  std::vector<ScaleFactor> supported_scale_factors;
#if defined(OS_IOS)
  display::Display display = display::Screen::GetScreen()->GetPrimaryDisplay();
  if (display.device_scale_factor() > 2.0) {
    DCHECK_EQ(3.0, display.device_scale_factor());
    supported_scale_factors.push_back(SCALE_FACTOR_300P);
  } else if (display.device_scale_factor() > 1.0) {
    DCHECK_EQ(2.0, display.device_scale_factor());
    supported_scale_factors.push_back(SCALE_FACTOR_200P);
  } else {
    supported_scale_factors.push_back(SCALE_FACTOR_100P);
  }
#else
  // On platforms other than iOS, 100P is always a supported scale factor.
  // For Windows we have a separate case in this function.
  supported_scale_factors.push_back(SCALE_FACTOR_100P);
#if defined(OS_MACOSX) || defined(OS_LINUX) || defined(OS_WIN)
  supported_scale_factors.push_back(SCALE_FACTOR_200P);
#endif
#endif
  ui::SetSupportedScaleFactors(supported_scale_factors);
}

void ResourceBundle::LoadChromeResources() {
  // Always load the 1x data pack first as the 2x data pack contains both 1x and
  // 2x images. The 1x data pack only has 1x images, thus passes in an accurate
  // scale factor to gfx::ImageSkia::AddRepresentation.
  if (IsScaleFactorSupported(SCALE_FACTOR_100P)) {
    AddDataPackFromPath(GetResourcesPakFilePath(
        "chrome_100_percent.pak"), SCALE_FACTOR_100P);
  }

  if (IsScaleFactorSupported(SCALE_FACTOR_200P)) {
    AddOptionalDataPackFromPath(GetResourcesPakFilePath(
        "chrome_200_percent.pak"), SCALE_FACTOR_200P);
  }
}

void ResourceBundle::AddDataPackFromPathInternal(
    const base::FilePath& path,
    ScaleFactor scale_factor,
    bool optional) {
  // Do not pass an empty |path| value to this method. If the absolute path is
  // unknown pass just the pack file name.
  DCHECK(!path.empty());

  base::FilePath pack_path = path;
  if (delegate_)
    pack_path = delegate_->GetPathForResourcePack(pack_path, scale_factor);

  // Don't try to load empty values or values that are not absolute paths.
  if (pack_path.empty() || !pack_path.IsAbsolute())
    return;

  std::unique_ptr<DataPack> data_pack(new DataPack(scale_factor));
  if (data_pack->LoadFromPath(pack_path)) {
    AddDataPack(std::move(data_pack));
  } else if (!optional) {
    LOG(ERROR) << "Failed to load " << pack_path.value()
               << "\nSome features may not be available.";
  }
}

// static
bool ResourceBundle::PNGContainsFallbackMarker(const unsigned char* buf,
                                               size_t size) {
  if (size < arraysize(kPngMagic) ||
      memcmp(buf, kPngMagic, arraysize(kPngMagic)) != 0) {
    // Data invalid or a JPEG.
    return false;
  }
  size_t pos = arraysize(kPngMagic);

  // Scan for custom chunks until we find one, find the IDAT chunk, or run out
  // of chunks.
  for (;;) {
    if (size - pos < kPngChunkMetadataSize)
      break;
    uint32_t length = 0;
    base::ReadBigEndian(reinterpret_cast<const char*>(buf + pos), &length);
    if (size - pos - kPngChunkMetadataSize < length)
      break;
    if (length == 0 &&
        memcmp(buf + pos + sizeof(uint32_t), kPngScaleChunkType,
               arraysize(kPngScaleChunkType)) == 0) {
      return true;
    }
    if (memcmp(buf + pos + sizeof(uint32_t), kPngDataChunkType,
               arraysize(kPngDataChunkType)) == 0) {
      // Stop looking for custom chunks, any custom chunks should be before an
      // IDAT chunk.
      break;
    }
    pos += length + kPngChunkMetadataSize;
  }
  return false;
}

}  // namespace ui
