// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/webstore_widget_private/webstore_widget_private_api.h"

#include <memory>
#include <utility>

#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/extensions/api/webstore_widget_private/app_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/webstore_widget_private.h"
#include "extensions/browser/extension_function_constants.h"

namespace extensions {
namespace api {

namespace {

const char kGoogleCastApiExtensionId[] = "mafeflapfdfljijmlienjedomfjfmhpd";

}  // namespace

WebstoreWidgetPrivateInstallWebstoreItemFunction::
    WebstoreWidgetPrivateInstallWebstoreItemFunction() {
}

WebstoreWidgetPrivateInstallWebstoreItemFunction::
    ~WebstoreWidgetPrivateInstallWebstoreItemFunction() {
}

ExtensionFunction::ResponseAction
WebstoreWidgetPrivateInstallWebstoreItemFunction::Run() {
  const std::unique_ptr<webstore_widget_private::InstallWebstoreItem::Params>
      params(
          webstore_widget_private::InstallWebstoreItem::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  if (params->item_id.empty())
    return RespondNow(Error("App ID empty."));

  bool allow_silent_install =
      extension()->id() == file_manager::kVideoPlayerAppId &&
      params->item_id == kGoogleCastApiExtensionId;
  if (params->silent_installation && !allow_silent_install)
    return RespondNow(Error("Silent installation not allowed."));

  const extensions::WebstoreStandaloneInstaller::Callback callback = base::Bind(
      &WebstoreWidgetPrivateInstallWebstoreItemFunction::OnInstallComplete,
      this);

  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents) {
    return RespondNow(
        Error(function_constants::kCouldNotFindSenderWebContents));
  }
  scoped_refptr<webstore_widget::AppInstaller> installer(
      new webstore_widget::AppInstaller(
          web_contents, params->item_id,
          Profile::FromBrowserContext(browser_context()),
          params->silent_installation, callback));
  // installer will be AddRef()'d in BeginInstall().
  installer->BeginInstall();

  return RespondLater();
}

void WebstoreWidgetPrivateInstallWebstoreItemFunction::OnInstallComplete(
    bool success,
    const std::string& error,
    extensions::webstore_install::Result result) {
  Respond(success ? NoArguments() : Error(error));
}

}  // namespace api
}  // namespace extensions
