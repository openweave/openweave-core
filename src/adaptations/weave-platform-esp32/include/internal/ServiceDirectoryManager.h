#ifndef SERVICE_DIRECTORY_MANAGER_H
#define SERVICE_DIRECTORY_MANAGER_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

namespace WeavePlatform {
namespace Internal {

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

extern ::nl::Weave::Profiles::ServiceDirectory::WeaveServiceManager ServiceDirectoryMgr;

extern WEAVE_ERROR InitServiceDirectoryManager(void);

#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

} // namespace Internal
} // namespace WeavePlatform

#endif // SERVICE_DIRECTORY_MANAGER_H
