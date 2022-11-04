Invoke-WebRequest https://aka.ms/vs/17/release/vc_redist.x64.exe -OutFile vc_redist_x64.exe
.\vc_redist_x64.exe /quiet
COPY-ITEM -Path @("C:\Windows\System32\msvcp140.dll", "C:\Windows\System32\vcruntime140.dll", "C:\Windows\System32\vcruntime140_1.dll") -Destination "publish\"
