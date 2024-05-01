Due to OS security restrictions, Magpie requires UIAccess privileges to support touch input. Obtaining this privilege necessitates meeting two conditions:

1. The application must possess a digital signature, and this signature must be verified by a certificate associated with a trusted root certificate authority store on the local machine.
2. TThe application must reside in a "secure location", such as the Program Files or System32 folders.

When enabling touch support, Magpie performs the following actions:

1. Adds a self-signed certificate to the trusted root certificate authority store.
2. Copies the TouchHelper.exe to `System32\Magpie`. During scaling, Magpie runs this program to enable touch support.

Both of these actions constitute significant changes to the OS, thus requiring administrator privileges. If touch support is no longer needed, this option should be disabled. Magpie will then revert these changes, leaving no traces in the OS.
