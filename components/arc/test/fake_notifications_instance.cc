// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/test/fake_notifications_instance.h"

#include "base/bind.h"
#include "base/bind_helpers.h"

namespace arc {

FakeNotificationsInstance::FakeNotificationsInstance() = default;
FakeNotificationsInstance::~FakeNotificationsInstance() = default;

void FakeNotificationsInstance::SendNotificationEventToAndroid(
    const std::string& key,
    mojom::ArcNotificationEvent event) {
  events_.emplace_back(key, event);
}

void FakeNotificationsInstance::CreateNotificationWindow(
    const std::string& key) {}

void FakeNotificationsInstance::CloseNotificationWindow(
    const std::string& key) {}

void FakeNotificationsInstance::OpenNotificationSettings(
    const std::string& key) {}

void FakeNotificationsInstance::OpenNotificationSnoozeSettings(
    const std::string& key) {}

void FakeNotificationsInstance::SetDoNotDisturbStatusOnAndroid(
    mojom::ArcDoNotDisturbStatusPtr status) {
  latest_do_not_disturb_status_ = std::move(status);
}

void FakeNotificationsInstance::CancelLongPress(const std::string& key) {}

void FakeNotificationsInstance::InitDeprecated(
    mojom::NotificationsHostPtr host_ptr) {
  Init(std::move(host_ptr), base::DoNothing());
}

void FakeNotificationsInstance::Init(mojom::NotificationsHostPtr host_ptr,
                                     InitCallback callback) {
  std::move(callback).Run();
}

const std::vector<std::pair<std::string, mojom::ArcNotificationEvent>>&
FakeNotificationsInstance::events() const {
  return events_;
}

const mojom::ArcDoNotDisturbStatusPtr&
FakeNotificationsInstance::latest_do_not_disturb_status() const {
  return latest_do_not_disturb_status_;
}

void FakeNotificationsInstance::PerformDeferredUserAction(uint32_t action_id) {}
void FakeNotificationsInstance::CancelDeferredUserAction(uint32_t action_id) {}
void FakeNotificationsInstance::SetLockScreenSettingOnAndroid(
    mojom::ArcLockScreenNotificationSettingPtr setting) {}

}  // namespace arc
