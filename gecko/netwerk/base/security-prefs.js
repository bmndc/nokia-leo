/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

pref("security.tls.version.min", 1);
pref("security.tls.version.max", 3);
pref("security.tls.version.fallback-limit", 3);
pref("security.tls.insecure_fallback_hosts", "");
pref("security.tls.unrestricted_rc4_fallback", false);

pref("security.ssl.treat_unsafe_negotiation_as_broken", false);
pref("security.ssl.require_safe_negotiation",  false);
pref("security.ssl.enable_ocsp_stapling", true);
pref("security.ssl.enable_false_start", true);
pref("security.ssl.false_start.require-npn", false);
pref("security.ssl.enable_npn", true);
pref("security.ssl.enable_alpn", true);

pref("security.ssl3.ecdhe_rsa_aes_128_gcm_sha256", true);
pref("security.ssl3.ecdhe_ecdsa_aes_128_gcm_sha256", true);
pref("security.ssl3.ecdhe_ecdsa_chacha20_poly1305_sha256", true);
pref("security.ssl3.ecdhe_rsa_chacha20_poly1305_sha256", true);
pref("security.ssl3.ecdhe_ecdsa_aes_256_gcm_sha384", true);
pref("security.ssl3.ecdhe_rsa_aes_256_gcm_sha384", true);
pref("security.ssl3.ecdhe_rsa_aes_128_sha", true);
pref("security.ssl3.ecdhe_ecdsa_aes_128_sha", true);
pref("security.ssl3.ecdhe_rsa_aes_256_sha", true);
pref("security.ssl3.ecdhe_ecdsa_aes_256_sha", true);
pref("security.ssl3.dhe_rsa_aes_128_sha", true);
pref("security.ssl3.dhe_rsa_aes_256_sha", true);
pref("security.ssl3.ecdhe_rsa_rc4_128_sha", true);
pref("security.ssl3.ecdhe_ecdsa_rc4_128_sha", true);
pref("security.ssl3.rsa_aes_128_sha", true);
pref("security.ssl3.rsa_aes_256_sha", true);
pref("security.ssl3.rsa_des_ede3_sha", true);
pref("security.ssl3.rsa_rc4_128_sha", true);
pref("security.ssl3.rsa_rc4_128_md5", true);

pref("security.content.signature.root_hash",
     "97:E8:BA:9C:F1:2F:B3:DE:53:CC:42:A4:E6:57:7E:D6:4D:F4:93:C2:47:B4:14:FE:A0:36:81:8D:38:23:56:0E");

pref("security.default_personal_cert",   "Ask Every Time");
pref("security.remember_cert_checkbox_default_setting", true);
pref("security.ask_for_password",        0);
pref("security.password_lifetime",       30);

pref("security.OCSP.enabled", 1);
pref("security.OCSP.require", false);
pref("security.OCSP.GET.enabled", false);

pref("security.pki.cert_short_lifetime_in_days", 10);
// NB: Changes to this pref affect CERT_CHAIN_SHA1_POLICY_STATUS telemetry.
// See the comment in CertVerifier.cpp.
// 3 = allow SHA-1 for certificates issued before 2016 or by an imported root.
pref("security.pki.sha1_enforcement_level", 3);

// security.pki.name_matching_mode controls how the platform matches hostnames
// to name information in TLS certificates. The possible values are:
// 0: always fall back to the subject common name if necessary (as in, if the
//    subject alternative name extension is either not present or does not
//    contain any DNS names or IP addresses)
// 1: fall back to the subject common name for certificates valid before 23
//    August 2016 if necessary
// 2: only use name information from the subject alternative name extension
#ifdef RELEASE_BUILD
pref("security.pki.name_matching_mode", 1);
#else
pref("security.pki.name_matching_mode", 2);
#endif

#ifdef ENABLE_U2F
pref("security.webauth.u2f", true);
pref("security.webauth.u2f_enable_softtoken", true);
pref("security.webauth.u2f_enable_usbtoken", true);
#else
pref("security.webauth.u2f", false);
pref("security.webauth.u2f_enable_softtoken", false);
pref("security.webauth.u2f_enable_usbtoken", false);
#endif

pref("security.ssl.errorReporting.enabled", true);
pref("security.ssl.errorReporting.url", "https://data.mozilla.com/submit/sslreports");
pref("security.ssl.errorReporting.automatic", false);
