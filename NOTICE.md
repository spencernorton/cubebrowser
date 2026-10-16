# NOTICE

CubeBrowser was created by Arial-Bold — https://github.com/Arial-Bold/Cubebrowser

This is a fork maintained for integration into Indigo, used with permission from
Arial-Bold. Changes here (dependency pinning, a refreshed CA bundle, parser
fixes, a fuzz suite and an Exit-to-Indigo option) are listed in the git history.

Third-party components the build fetches:

  mbed TLS — https://github.com/Mbed-TLS/mbedtls
    Apache-2.0 or GPL-2.0-or-later. Pinned and checksum-verified by
    get-mbedtls.sh; not stored in this tree.

  stb_image — https://github.com/nothings/stb
    Public domain (or MIT). Pinned and checksum-verified by get-mbedtls.sh.

  The CA bundle in sd/gcbrowser/cacert.pem is Mozilla's trust program,
  distributed via https://curl.se/ca/cacert.pem (MPL-2.0).
