// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/shell_dialogs/select_file_dialog_win.h"

#include <algorithm>
#include <memory>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/registry.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/shell_dialogs/base_shell_dialog_win.h"
#include "ui/shell_dialogs/execute_select_file_win.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/strings/grit/ui_strings.h"

namespace ui {

namespace {

// Get the file type description from the registry. This will be "Text Document"
// for .txt files, "JPEG Image" for .jpg files, etc. If the registry doesn't
// have an entry for the file type, we return false, true if the description was
// found. 'file_ext' must be in form ".txt".
bool GetRegistryDescriptionFromExtension(const base::string16& file_ext,
                                         base::string16* reg_description) {
  DCHECK(reg_description);
  base::win::RegKey reg_ext(HKEY_CLASSES_ROOT, file_ext.c_str(), KEY_READ);
  base::string16 reg_app;
  if (reg_ext.ReadValue(NULL, &reg_app) == ERROR_SUCCESS && !reg_app.empty()) {
    base::win::RegKey reg_link(HKEY_CLASSES_ROOT, reg_app.c_str(), KEY_READ);
    if (reg_link.ReadValue(NULL, reg_description) == ERROR_SUCCESS)
      return true;
  }
  return false;
}

// Set up a filter for a Save/Open dialog, |ext_desc| as the text descriptions
// of the |file_ext| types (optional), and (optionally) the default 'All Files'
// view. The purpose of the filter is to show only files of a particular type in
// a Windows Save/Open dialog box. The resulting filter is returned. The filter
// created here are:
//   1. only files that have 'file_ext' as their extension
//   2. all files (only added if 'include_all_files' is true)
// If a description is not provided for a file extension, it will be retrieved
// from the registry. If the file extension does not exist in the registry, it
// will be omitted from the filter, as it is likely a bogus extension.
std::vector<FileFilterSpec> FormatFilterForExtensions(
    const std::vector<base::string16>& file_ext,
    const std::vector<base::string16>& ext_desc,
    bool include_all_files) {
  const base::string16 all_ext = L"*.*";
  const base::string16 all_desc =
      l10n_util::GetStringUTF16(IDS_APP_SAVEAS_ALL_FILES);

  DCHECK(file_ext.size() >= ext_desc.size());

  if (file_ext.empty())
    include_all_files = true;

  std::vector<FileFilterSpec> result;

  // Precompute the final size of the resulting vector.
  size_t final_size = file_ext.size() + (include_all_files ? 1 : 0);
  result.resize(final_size);

  for (size_t i = 0; i < file_ext.size(); ++i) {
    base::string16 ext = file_ext[i];
    base::string16 desc;
    if (i < ext_desc.size())
      desc = ext_desc[i];

    if (ext.empty()) {
      // Force something reasonable to appear in the dialog box if there is no
      // extension provided.
      include_all_files = true;
      continue;
    }

    if (desc.empty()) {
      DCHECK(ext.find(L'.') != base::string16::npos);
      base::string16 first_extension = ext.substr(ext.find(L'.'));
      size_t first_separator_index = first_extension.find(L';');
      if (first_separator_index != base::string16::npos)
        first_extension = first_extension.substr(0, first_separator_index);

      // Find the extension name without the preceeding '.' character.
      base::string16 ext_name = first_extension;
      size_t ext_index = ext_name.find_first_not_of(L'.');
      if (ext_index != base::string16::npos)
        ext_name = ext_name.substr(ext_index);

      if (!GetRegistryDescriptionFromExtension(first_extension, &desc)) {
        // The extension doesn't exist in the registry. Create a description
        // based on the unknown extension type (i.e. if the extension is .qqq,
        // the we create a description "QQQ File (.qqq)").
        include_all_files = true;
        desc =
            l10n_util::GetStringFUTF16(IDS_APP_SAVEAS_EXTENSION_FORMAT,
                                       base::i18n::ToUpper(ext_name), ext_name);
      }
      if (desc.empty())
        desc = L"*." + ext_name;
    }

    result[i] = {desc, ext};
  }

  if (include_all_files)
    result.back() = {all_desc, all_ext};

  return result;
}

// Implementation of SelectFileDialog that shows a Windows common dialog for
// choosing a file or folder.
class SelectFileDialogImpl : public ui::SelectFileDialog,
                             public ui::BaseShellDialogImpl {
 public:
  SelectFileDialogImpl(
      Listener* listener,
      std::unique_ptr<ui::SelectFilePolicy> policy,
      const ExecuteSelectFileCallback& execute_select_file_callback);

  // BaseShellDialog implementation:
  bool IsRunning(gfx::NativeWindow owning_window) const override;
  void ListenerDestroyed() override;

 protected:
  // SelectFileDialog implementation:
  void SelectFileImpl(Type type,
                      const base::string16& title,
                      const base::FilePath& default_path,
                      const FileTypeInfo* file_types,
                      int file_type_index,
                      const base::FilePath::StringType& default_extension,
                      gfx::NativeWindow owning_window,
                      void* params) override;

 private:
  ~SelectFileDialogImpl() override;

  struct SelectFolderDialogOptions {
    const wchar_t* default_path;
    bool is_upload;
  };

  // Returns the result of the select file operation to the listener.
  void OnSelectFileExecuted(Type type,
                            std::unique_ptr<RunState> run_state,
                            void* params,
                            std::pair<std::vector<base::FilePath>, int> result);

  bool HasMultipleFileTypeChoicesImpl() override;

  // Returns the filter to be used while displaying the open/save file dialog.
  // This is computed from the extensions for the file types being opened.
  // |file_types| can be NULL in which case the returned filter will be empty.
  static std::vector<FileFilterSpec> GetFilterForFileTypes(
      const FileTypeInfo* file_types);

  bool has_multiple_file_type_choices_;
  ExecuteSelectFileCallback execute_select_file_callback_;

  DISALLOW_COPY_AND_ASSIGN(SelectFileDialogImpl);
};

SelectFileDialogImpl::SelectFileDialogImpl(
    Listener* listener,
    std::unique_ptr<ui::SelectFilePolicy> policy,
    const ExecuteSelectFileCallback& execute_select_file_callback)
    : SelectFileDialog(listener, std::move(policy)),
      BaseShellDialogImpl(),
      has_multiple_file_type_choices_(false),
      execute_select_file_callback_(execute_select_file_callback) {}

SelectFileDialogImpl::~SelectFileDialogImpl() = default;

void SelectFileDialogImpl::SelectFileImpl(
    Type type,
    const base::string16& title,
    const base::FilePath& default_path,
    const FileTypeInfo* file_types,
    int file_type_index,
    const base::FilePath::StringType& default_extension,
    gfx::NativeWindow owning_window,
    void* params) {
  has_multiple_file_type_choices_ =
      file_types ? file_types->extensions.size() > 1 : true;

  std::vector<FileFilterSpec> filter = GetFilterForFileTypes(file_types);
  HWND owner = owning_window && owning_window->GetRootWindow()
                   ? owning_window->GetHost()->GetAcceleratedWidget()
                   : NULL;

  std::unique_ptr<RunState> run_state = BeginRun(owner);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      run_state->dialog_task_runner;
  base::PostTaskAndReplyWithResult(
      task_runner.get(), FROM_HERE,
      base::BindOnce(execute_select_file_callback_, type, title, default_path,
                     filter, file_type_index, default_extension, owner),
      base::BindOnce(&SelectFileDialogImpl::OnSelectFileExecuted, this, type,
                     std::move(run_state), params));
}

bool SelectFileDialogImpl::HasMultipleFileTypeChoicesImpl() {
  return has_multiple_file_type_choices_;
}

bool SelectFileDialogImpl::IsRunning(gfx::NativeWindow owning_window) const {
  if (!owning_window->GetRootWindow())
    return false;
  HWND owner = owning_window->GetHost()->GetAcceleratedWidget();
  return listener_ && IsRunningDialogForOwner(owner);
}

void SelectFileDialogImpl::ListenerDestroyed() {
  // Our associated listener has gone away, so we shouldn't call back to it if
  // our worker thread returns after the listener is dead.
  listener_ = NULL;
}

void SelectFileDialogImpl::OnSelectFileExecuted(
    Type type,
    std::unique_ptr<RunState> run_state,
    void* params,
    std::pair<std::vector<base::FilePath>, int> result) {
  const std::vector<base::FilePath>& paths = result.first;
  const int index = result.second;

  if (listener_) {
    // The paths vector is empty when the user cancels the dialog.
    if (paths.empty()) {
      listener_->FileSelectionCanceled(params);
    } else {
      switch (type) {
        case SELECT_FOLDER:
        case SELECT_UPLOAD_FOLDER:
        case SELECT_EXISTING_FOLDER:
        case SELECT_SAVEAS_FILE:
        case SELECT_OPEN_FILE:
          DCHECK_EQ(paths.size(), 1u);
          listener_->FileSelected(paths[0], index, params);
          break;
        case SELECT_OPEN_MULTI_FILE:
          listener_->MultiFilesSelected(paths, params);
          break;
        case SELECT_NONE:
          NOTREACHED();
      }
    }
  }

  EndRun(std::move(run_state));
}

// static
std::vector<FileFilterSpec> SelectFileDialogImpl::GetFilterForFileTypes(
    const FileTypeInfo* file_types) {
  if (!file_types)
    return std::vector<FileFilterSpec>();

  std::vector<base::string16> exts;
  for (size_t i = 0; i < file_types->extensions.size(); ++i) {
    const std::vector<base::string16>& inner_exts = file_types->extensions[i];
    base::string16 ext_string;
    for (size_t j = 0; j < inner_exts.size(); ++j) {
      if (!ext_string.empty())
        ext_string.push_back(L';');
      ext_string.append(L"*.");
      ext_string.append(inner_exts[j]);
    }
    exts.push_back(ext_string);
  }
  return FormatFilterForExtensions(exts,
                                   file_types->extension_description_overrides,
                                   file_types->include_all_files);
}

}  // namespace

SelectFileDialog* CreateWinSelectFileDialog(
    SelectFileDialog::Listener* listener,
    std::unique_ptr<SelectFilePolicy> policy,
    const ExecuteSelectFileCallback& execute_select_file_callback) {
  return new SelectFileDialogImpl(listener, std::move(policy),
                                  execute_select_file_callback);
}

SelectFileDialog* CreateSelectFileDialog(
    SelectFileDialog::Listener* listener,
    std::unique_ptr<SelectFilePolicy> policy) {
  return CreateWinSelectFileDialog(listener, std::move(policy),
                                   base::BindRepeating(&ui::ExecuteSelectFile));
}

}  // namespace ui
