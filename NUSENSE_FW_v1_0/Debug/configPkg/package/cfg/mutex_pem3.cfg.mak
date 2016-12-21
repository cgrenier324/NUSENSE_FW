# invoke SourceDir generated makefile for mutex.pem3
mutex.pem3: .libraries,mutex.pem3
.libraries,mutex.pem3: package/cfg/mutex_pem3.xdl
	$(MAKE) -f C:\Users\cgren\workspace_v6_1_3\NUSENSE_FW_v1_0/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\cgren\workspace_v6_1_3\NUSENSE_FW_v1_0/src/makefile.libs clean

