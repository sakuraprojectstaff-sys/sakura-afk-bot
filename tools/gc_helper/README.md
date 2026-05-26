# AFKbot GC Helper

This helper is launched by AFKbot Loader when Party Builder uses the Game Coordinator backend.

## Current flow

1. In the loader run Verify Windows, Identify IDs, and Build Parties.
2. Open the GC Accounts tab.
3. Fill Steam login, password, and optional shared_secret for every detected account.
4. Click Save selected for each account.
5. Party Builder -> GC invite bots / GC accept.

The loader stores credentials in `data/config/gc_accounts_secure.cfg` protected by Windows DPAPI.
Before launching the helper, the loader creates a temporary plaintext runtime file:

`data/config/gc_accounts_runtime.json`

The helper reads it and deletes it immediately after loading.

Do not upload `gc_accounts_secure.cfg` or `gc_accounts_runtime.json` anywhere.

## Install

```bat
cd tools\gc_helper
npm install
```

Do not run `npm audit fix --force` unless you are ready to retest the helper dependencies.
