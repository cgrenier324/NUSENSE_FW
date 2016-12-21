## THIS IS A GENERATED FILE -- DO NOT EDIT
.configuro: .libraries,em3 linker.cmd package/cfg/mutex_pem3.oem3

# To simplify configuro usage in makefiles:
#     o create a generic linker command file name 
#     o set modification times of compiler.opt* files to be greater than
#       or equal to the generated config header
#
linker.cmd: package/cfg/mutex_pem3.xdl
	$(SED) 's"^\"\(package/cfg/mutex_pem3cfg.cmd\)\"$""\"C:/Users/cgren/workspace_v6_1_3/NUSENSE_FW_v1_0/Debug/configPkg/\1\""' package/cfg/mutex_pem3.xdl > $@
	-$(SETDATE) -r:max package/cfg/mutex_pem3.h compiler.opt compiler.opt.defs
