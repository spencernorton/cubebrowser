# Start-page ZIP preference

This is a single first-party preference, not a general cookie or login store.
The site can use an ordinary no-JavaScript form and respond with:

```http
Set-Cookie: indigo_zip=80202; Path=/indigo/start/; Max-Age=31536000; Secure; HttpOnly; SameSite=Lax
```

The value is exactly five ASCII digits, preserving leading zeroes. Domain
attributes are rejected: the host is exactly `norvitech.com`, HTTPS port 443,
with the `/indigo/start/` path prefix. All flags above are required; Max-Age
must be 0–31536000 seconds. Quotes around the value are accepted. Unknown
cookies are ignored. To clear the preference:

```http
Set-Cookie: indigo_zip=; Path=/indigo/start/; Max-Age=0; Secure; HttpOnly; SameSite=Lax
```

Read and write only after certificate verification. Send `Cookie:
indigo_zip=80202` on subsequent safe GET page navigations in scope and on
in-scope image/stylesheets initiated by a start page. Other pages' subresources,
other origins/paths, unverified TLS, explicit download streams and POST requests receive no
preference header. A save/clear POST must carry its submitted value or action
in the body; its verified response may set the preference and redirect to a
GET. Same-origin start-page images bypass the image helper, so a small weather
card loads directly and keeps its preference out of that helper's requests.

The network worker owns the in-memory value. Before it starts, load the tiny
versioned `gcbrowser/start.pref` from the selected card (64-byte input ceiling).
Atomic saves use `start.tmp`. Expiry and malformed records are rejected, and
clock rollback cannot extend the lifetime beyond a year. A save failure keeps
the value for this visit and shows a toast; it never claims card persistence.
`sd=0` still prevents card access.

The required host suite checks origin/path/trust, flags, digit/expiry bounds,
card reload, quoted clearing, save failure and malformed records. The required
TLS emulator route drives one produced DOL through three boots, verifies its
request headers and reads back the actual FAT record after save and clear.
The fixture CA and key are temporary test assets, never package roots.
