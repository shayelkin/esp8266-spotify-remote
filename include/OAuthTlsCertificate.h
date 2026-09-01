/**
 * Self-signed TLS credentials for the on-device OAuth callback server.
 *
 * The certificate has a DNS SAN for *.local, which covers the default mDNS
 * hostname as well as a custom espotifierNodeName. Browsers will require the
 * user to accept the certificate warning because it is self-signed.
 *
 * These credentials only protect the transport after the warning has been
 * accepted. Generate a replacement key pair before distributing a device in
 * an environment where other people may use it.
 */
#pragma once

static const char OAUTH_TLS_CERTIFICATE[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIBqjCCAVCgAwIBAgIUOkbrV1aTLRIOerIkRw0rxzJxmoQwCgYIKoZIzj0EAwIw
GDEWMBQGA1UEAwwNZXNwODI2Ni5sb2NhbDAeFw0yNjA5MDExOTIwMjZaFw0zNjA4
MjkxOTIwMjZaMBgxFjAUBgNVBAMMDWVzcDgyNjYubG9jYWwwWTATBgcqhkjOPQIB
BggqhkjOPQMBBwNCAATgN2gu5gSU7NRHsLUembynddod+SIt5rtBEW72QH9EQWyZ
jTy0w+aHEhm//l42TOFBkAiyBgDWhL1WbMTLFUswo3gwdjAdBgNVHQ4EFgQUw7IL
ZWoicK6/TqzUJrUM8+W0Y2EwHwYDVR0jBBgwFoAUw7ILZWoicK6/TqzUJrUM8+W0
Y2EwDwYDVR0TAQH/BAUwAwEB/zAjBgNVHREEHDAaggcqLmxvY2Fsgglsb2NhbGhv
c3SHBH8AAAEwCgYIKoZIzj0EAwIDSAAwRQIhAP6nvDyERJpgZKoJ/3QBZaEC7Faz
EZ77GIxg2f+UOOEqAiBdiSVsQq9o7YOQ6FkqAyFiEj125j94rIijVJNy+HLnYg==
-----END CERTIFICATE-----
)EOF";

static const char OAUTH_TLS_PRIVATE_KEY[] = R"EOF(
-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg3h0AovViNgunPnUq
2uONsOoHOQRlHAYOwmMdZkdDr2ehRANCAATgN2gu5gSU7NRHsLUembynddod+SIt
5rtBEW72QH9EQWyZjTy0w+aHEhm//l42TOFBkAiyBgDWhL1WbMTLFUsw
-----END PRIVATE KEY-----
)EOF";
