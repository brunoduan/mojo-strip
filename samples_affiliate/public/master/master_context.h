#ifndef SAMPLES_PUBLIC_MASTER_MASTER_CONTEXT_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_CONTEXT_H_

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>

#include "base/supports_user_data.h"
#include "samples/common/export.h"
#include "services/service_manager/embedder/embedded_service_info.h"

namespace base {
class FilePath;
}

namespace service_manager {
class Connector;
}

namespace samples {

class ServiceManagerConnection;

class SAMPLES_EXPORT MasterContext : public base::SupportsUserData {
 public:
  MasterContext();
  ~MasterContext() override;

  using StaticServiceMap =
      std::map<std::string, service_manager::EmbeddedServiceInfo>;

  // Registers per-browser-context services to be loaded in the browser process
  // by the Service Manager.
  virtual void RegisterInProcessServices(StaticServiceMap* services) {}

  // Returns a unique string associated with this browser context.
  virtual const std::string& UniqueId() const;

  static void NotifyWillBeDestroyed(MasterContext* master_context);

  // Makes the Service Manager aware of this MasterContext, and assigns a user
  // ID number to it. Should be called for each MasterContext created.
  static void Initialize(MasterContext* browser_context,
                         const base::FilePath& path);

  // Returns a Service User ID associated with this MasterContext. This ID is
  // not persistent across runs. See
  // services/service_manager/public/mojom/connector.mojom. By default,
  // this user id is randomly generated when Initialize() is called.
  static const std::string& GetServiceUserIdFor(
      MasterContext* browser_context);

  // Returns the MasterContext associated with |user_id|, or nullptr if no
  // MasterContext exists for that |user_id|.
  static MasterContext* GetMasterContextForServiceUserId(
      const std::string& user_id);

  // Returns a Connector associated with this MasterContext, which can be used
  // to connect to service instances bound as this user.
  static service_manager::Connector* GetConnectorFor(
      MasterContext* browser_context);
  static ServiceManagerConnection* GetServiceManagerConnectionFor(
      MasterContext* browser_context);

 private:
  const std::string unique_id_;
  bool was_notify_will_be_destroyed_called_ = false;
};

}

#endif // SAMPLES_PUBLIC_MASTER_MASTER_CONTEXT_H_
