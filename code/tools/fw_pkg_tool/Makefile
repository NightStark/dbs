CFLAGS 	=-std=gnu99 -fstrict-aliasing -fP
LDFLAGS  =  -lcrypto

FW_BUILD_SRC = fw.c rsa_sign_generate.c
FW_EXTRACT_SRC = fw.c rsa_sign_verify.c
fw:
	gcc $(FW_BUILD_SRC) -o fw_builder -DBUILD_PKG  -DFW_RSA_USE_BUF -lcrypto
	gcc $(FW_EXTRACT_SRC) -o fw_extracter -DEXT_PKG -DFW_RSA_USE_BUF $(LDFLAGS)


