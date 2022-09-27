if(NOT SCORE_MSSTORE_DEPLOYMENT)
  return()
endif()

file(CONFIGURE
  OUTPUT
    "${PROJECT_BINARY_DIR}/score.exe.manifest"
  CONTENT
  [=[<?xml version="1.0" encoding="UTF-8" standalone="yes"?> 
  <assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"> 
    <application xmlns="urn:schemas-microsoft-com:asm.v3"> 
       <windowsSettings> <dpiAware      xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/PM</dpiAware>                     </windowsSettings> 
       <windowsSettings> <dpiAwareness  xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2,PerMonitor</dpiAwareness> </windowsSettings> 
       <windowsSettings> <longPathAware xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">true</longPathAware>                   </windowsSettings> 
    </application> 
    <trustInfo xmlns="urn:schemas-microsoft-com:asm.v2"> 
      <security> 
        <requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3"> 
          <requestedExecutionLevel level="asInvoker" uiAccess="false" /> 
        </requestedPrivileges> 
      </security> 
    </trustInfo> 
    <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1"> 
      <application> 
        <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}" /> 
        <supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}" /> 
        <supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}" /> 
        <supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}" /> 
      </application> 
    </compatibility> 
  </assembly>   
  ]=]
)

install(
  FILES
    "${PROJECT_BINARY_DIR}/score.exe.manifest"
  DESTINATION
    "."
)

install(
  DIRECTORY
    "${CMAKE_CURRENT_LIST_DIR}/Deployment/Windows/store/images"
  DESTINATION
    "."
)


configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/Deployment/Windows/store/manifests/Package.appxmanifest.in"
  "${PROJECT_BINARY_DIR}/manifests/Package.appxmanifest"
  )

install(
  FILES
    "${PROJECT_BINARY_DIR}/manifests/Package.appxmanifest"
  DESTINATION
    "manifests/"
)