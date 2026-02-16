
import sys

# Root public key from host.py
root_mod_int = 0xdde2a6167e5123bf23c2281118ceb6eb9833bb92a325d55affcf51620eeec930663c7a3d6e48b585c6f5821deeff42b4b88ecaad8d6f33b009f32c3c8ad69e3db74d02b484d58f73552a2419aaf20cf8a84d2369400b84a666ee939c8532020aee6397920c3d24d774c2dd265d8e7a9965a241cc46c6550816778afd3ce8d403dbd7899870494f0639cc68b6c4ba18f3c7afa621843a6b728de5a3a48b365816f365f316e3b96b6d2564378e020facb97ed3107c180c23930ae4d70e798ac1d4b949d67a4915e328a9286f47bf7b3806782732516f5b5764d2d8bf3e09ab1e19cbe3f7eaca2df68325955fbbb03f837e8962360131f6a7a0a8181f6d0b747a378a18674040d4b19e682b5f02cf7aedf7594645f2c1652754b771182ad84814b52320ebc7b14194b87d998f9b00f8b68e8025a105d83310278c5a4c67a0b1d94c88da5691a86e09591d5436d0f35fe96a6e5597a9a7836956e3adbb43be6865943d7b0b45078782935bdb4e38daa2ef2213c0e0ce6596743a7def70b9ac205183f22da0785696bd63027522d543e4d0c1778b6947d9753c6f6d97096516d02483353d4a99815710cf09156205498ef028d751f5d311759142c4eda62f08cb3ef9504dfbf1e0c1c03d3f9f7b59eee6c2a00bff8ff7e8e8aa0248a3a955ccef2da56a10f19518a8d07a0e9cf6f439a7e91841d0113e833f92702f00a92ed0045a62f640a1
root_pub_exp = 65537

# Partner's Root Modulus
partner_mod_int = 0xc03658d5059ff69f8cb29a9c3668b82a28c4364d4e33cedbcf2bdd38da01dc11f94aba6a70ebe1c048c4a72143a466f5c0db746ef3d8fe6f424b1c13400ef56ea1138d454ffc3e0ae24883bc2dbd79f1e242bc036b5c111d386627bf7c551d7ae001d68c15cbe7a787f1792d23ab20182d071a04236f5255cc6dd38ddbce832471cac0caa0caceb57c261c3c3eaabeed1682e75a6b7574e5ffcecfea9995287f340ca60bb82b09007a15b905cb13a794ea0e0411e5fbb0d3a87c687fb6f9cd62671ab6fd849dceb9998360e29533438dba50c4296f33388831514fc9be260480fab39ddb72ab7c98010acc8a043c2a8f395b2d7c78716fc25fc83ed4c155a7d9962d08c00a995487736c9b6b65c2d98b1cc2629bad4981aa02c24f7bc1e6094ce09d05f1601288aa6baad7b440c7a25337cb22cd890dc16b24697f8bcd0752a3a468e7c5dd9c841aa4a3da3b4dd7af5ee3696c764bf8a7d4a69c998f87850cd5fb18662b0798389a013f4e3256be0cf65a7a43129e0c5944aaf82001c4599d64c8ccfef984215663178d6c2ede369709728ed0e2e3bd18292d3bf9430a50b54de9b79371768af7523aec03d05c2fce7a7eb831c0856baf77da792220c2d643912ea9313ac876ded97dc2de2a8ec50e25962e31ecff0d3b0f8e8381e7be83b6cd9b49ade86cea0ac523fd626732e51dbe6a6120cf08df6e1ae517576bf0782bd5

# Signature from log (LITTLE ENDIAN attempt, but the bytes are the source truth)
# The user's log printed: 294DF33E... which is the raw bytes from the UART.
# We will use this raw hex string.
sig_hex = "294DF33E4B59AAAC974F3633AFFE5F2FBE2674F923F8BDEB0B4E8CD5A4BA460807AEA7FCFC11A8D81495C4A22C63DE12E8736960845C71EF442F57258517D80D32D0785DB29304C6BDC17C95924B9CC5CD8512A8E175C078ACCAA9516483B1F6A0A240FA33A4F08F94C4A8EE299C053F60C1C4CBAC8C17EF9972942F9BC6E603"
# Note: The log output was truncated or split. I will take the FULL signature from the earlier "Received Kernel Certificate Signature" log in the previous turn if possible, but the one in this turn looks incomplete (only ~128 bytes?). 
# Wait, the log shows: "Attest Signature:294D...E603". Let's check the length.
# Length of hex string: 256 chars = 128 bytes. This is WAY too short for RSA-4096 (512 bytes).

# Ah, I see "Received Kernel Certificate Signature" in the first log which was 512 bytes (1024 hex chars).
# D4F9C3DE...
# That was the one from the *Big Endian* run.
# The "Attest Signature" in the second log is also short.

# Let's use the BIG signature from the FIRST log, as that captured the full 512 bytes output by the Security Kernel.
full_sig_hex = "D4F9C3DEADD11BD8713774AD8962DE09748E192C9FC5C6D74807A78F15C0E50D9D61B97C6CE8D7942DB4A068E7311965BF37A51E276F0384D4A548AC5457894A4520CD6EEFBCE6170F175259D3EF4ED5ED0C1E18AABF743C5133521E610EF1D775BAB3357DCD0E4E108AA7C0C8E66681CE6F8C01AF41E49EBB8E1E8A9085A322E654C6CE2C15F24DF0EE36A694CAE0C902575AD3A068F2996BCAC1F400967076C991B416107960D631DA8FE7E37A6A7FB4EBE701B8C5702F0A28F049960304EAD479700DF53825462843CE660D716ED5AB8CDC414344DA26CF113E8972C1170017DE2E4628D8EE721DD7968F991B00A426B8D8A8E7D37F27F0E91C8336DD7C03E092C23C397A16D7BA1DABF8AA080735D23135D00823C0FD938B79C654B00A2546940FDA56C3A8605F8976BD4F0BEB38C71C874EC75C3E8DAD5DF1F582A28E604B0CA1FD95032062AAE2D27FC9AFFAAC6A7DAFFE55DA9E4A3FA2D3FBF34683A38696A2546FED1EDB8068CBE0E613900C4F89CEDAABB40350D4D8E4F22806C55083B146DBA3E3E19FEC0592DE765BF154D678D0EB1E2B9E5864FC2BA344246EC1AE36784BF739B947973C0B2E719ADE5895D443A7E4F800FA0D336DA28C319ADA6B18A4D44E0C35EE31CA23BE6D065A0FDC55E93A05EBAE8324C254436AFE88A090D52E22AEEDE22119CBFCD982645FB437E2A29A2B89CB315B2A7BEAACB464AF"

def check_sig(name, sig_bytes, endian, mod):
    try:
        sig_int = int.from_bytes(sig_bytes, byteorder=endian, signed=False)
        decrypted = pow(sig_int, root_pub_exp, mod)
        dec_hex = hex(decrypted)[2:] # remove 0x
        if len(dec_hex) % 2 != 0: dec_hex = "0" + dec_hex
        
        # Check for PKCS#1 v1.5 padding: 00 01 FF FF ...
        # Since pow() returns an int, the leading 00 is implied if the number is smaller than modulus.
        # But for RSA-4096 (512 bytes), the modulus is 512 bytes.
        # If the result starts with '1ffff...', it corresponds to '00 01 ff ff'.
        
        print(f"--- {name} ({endian}) ---")
        print(f"Decrypted (first 32 chars): {dec_hex[:32]}")
        
        if dec_hex.startswith("1ffff"):
             print(">> MATCH: Valid PKCS#1 v1.5 padding found!")
             return True
        return False
    except Exception as e:
        print(f"Error in {name}: {e}")
        return False

sig_bytes = bytes.fromhex(full_sig_hex)

# 1. Standard Big Endian
check_sig("Standard", sig_bytes, 'big', root_mod_int)

# 2. Standard Little Endian
check_sig("Standard", sig_bytes, 'little', root_mod_int)

# 3. Reversed Modulus (Assuming Modulus is LE in Firmware)
# Convert root_mod_int to bytes (Big Endian), then reverse, then back to int.
num_bytes = (root_mod_int.bit_length() + 7) // 8
root_mod_bytes = root_mod_int.to_bytes(num_bytes, byteorder='big')
root_mod_le = int.from_bytes(root_mod_bytes, byteorder='little')

check_sig("Swapped Modulus", sig_bytes, 'big', root_mod_le)
check_sig("Swapped Modulus", sig_bytes, 'little', root_mod_le)

# 4. Partner Key (Big Endian)
check_sig("Partner Key", sig_bytes, 'big', partner_mod_int)
check_sig("Partner Key", sig_bytes, 'little', partner_mod_int)
