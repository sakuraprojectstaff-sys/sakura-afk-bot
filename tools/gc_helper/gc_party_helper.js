'use strict';

const fs = require('fs');
const path = require('path');
const SteamUser = require('steam-user');
const SteamTotp = require('steam-totp');
const { Dota2User } = require('dota2-user');
let Protos = {};
try { Protos = require('dota2-user/protobufs'); } catch (_) {}

const APPID_DOTA2 = 570;
const MSG_INVITE_TO_PARTY = 4501;
const MSG_INVITATION_CREATED = 4502;
const MSG_PARTY_INVITE_RESPONSE = 4503;

function arg(name, fallback = '') {
  const idx = process.argv.indexOf(`--${name}`);
  if (idx >= 0 && idx + 1 < process.argv.length) return process.argv[idx + 1];
  return fallback;
}

function argBool(name, fallback = false) {
  const v = String(arg(name, fallback ? '1' : '0')).toLowerCase();
  return v === '1' || v === 'true' || v === 'yes' || v === 'on';
}

function sleep(ms) { return new Promise(resolve => setTimeout(resolve, ms)); }
function now() { return new Date().toISOString().replace('T', ' ').replace('Z', ''); }
function log(msg) { console.log(`[${now()}] ${msg}`); }
function fail(msg, code = 1) { console.error(`[${now()}] ${msg}`); process.exit(code); }

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

function normalizeSteam64(value) {
  if (value === undefined || value === null) return '';
  return String(value).trim();
}

function loadAccountMap(accountsFile) {
  const cfg = readJson(accountsFile);
  const map = new Map();
  for (const account of (cfg.accounts || [])) {
    const steam64 = normalizeSteam64(account.steam64);
    if (!steam64) continue;
    map.set(steam64, account);
  }
  return { cfg, map };
}

function accountLogName(account) {
  return account.label || account.sandbox || account.accountName || account.steam64 || 'unknown';
}

function steamGuardCode(account) {
  if (account.sharedSecret && account.sharedSecret !== 'OPTIONAL_STEAM_GUARD_SHARED_SECRET') {
    return SteamTotp.generateAuthCode(account.sharedSecret);
  }
  if (account.twoFactorCode && account.twoFactorCode !== 'OPTIONAL_ONE_TIME_CODE') {
    return account.twoFactorCode;
  }
  return undefined;
}

function messageId(name, fallback) {
  if (Protos.EGCBaseMsg && Protos.EGCBaseMsg[name] !== undefined) return Protos.EGCBaseMsg[name];
  if (Protos.EGCBaseMsg && Protos.EGCBaseMsg[`k_EMsg${name}`] !== undefined) return Protos.EGCBaseMsg[`k_EMsg${name}`];
  return fallback;
}

const GC_INVITE_TO_PARTY = messageId('k_EMsgGCInviteToParty', MSG_INVITE_TO_PARTY);
const GC_INVITATION_CREATED = messageId('k_EMsgGCInvitationCreated', MSG_INVITATION_CREATED);
const GC_PARTY_INVITE_RESPONSE = messageId('k_EMsgGCPartyInviteResponse', MSG_PARTY_INVITE_RESPONSE);

class DotaGcSession {
  constructor(account, role) {
    this.account = account;
    this.role = role;
    this.client = new SteamUser({ dataDirectory: path.join(__dirname, 'sentry', normalizeSteam64(account.steam64) || account.accountName || 'default') });
    this.dota = new Dota2User(this.client);
    this.ready = false;
    this.lastPartyId = null;
    this.invites = [];
  }

  async connect(timeoutMs = 90000) {
    const account = this.account;
    const name = accountLogName(account);
    if (!account.accountName || !account.password || account.accountName.startsWith('PUT_') || account.password.startsWith('PUT_')) {
      throw new Error(`Steam credentials missing for ${name}`);
    }

    this.client.on('error', err => log(`[GC:${name}] Steam error: ${err.message || err}`));
    this.client.on('disconnected', (eresult, msg) => log(`[GC:${name}] disconnected eresult=${eresult || ''} ${msg || ''}`));
    this.client.on('steamGuard', (domain, callback) => {
      const code = steamGuardCode(account);
      if (!code) {
        log(`[GC:${name}] Steam Guard requested. Put twoFactorCode/sharedSecret into gc_accounts.json and rerun.`);
        return callback('');
      }
      callback(code);
    });

    this.client.on('loggedOn', () => {
      log(`[GC:${name}] Steam logged on. Starting Dota GC session...`);
      this.client.setPersona(SteamUser.EPersonaState.Online);
      this.client.gamesPlayed([APPID_DOTA2]);
    });

    this.dota.router.on(GC_INVITATION_CREATED, data => {
      const partyId = data.partyId || data.party_id || data.groupId || data.group_id || data.partyID || null;
      const sender = data.steamId || data.steam_id || data.senderId || data.sender_id || '';
      this.lastPartyId = partyId;
      this.invites.push({ partyId, sender, raw: data });
      log(`[GC:${name}] invitation created party=${partyId || 'unknown'} sender=${sender || 'unknown'}`);
    });

    this.dota.on('connectedToGC', () => {
      this.ready = true;
      log(`[GC:${name}] connected to Dota GC`);
    });

    this.client.logOn({
      accountName: account.accountName,
      password: account.password,
      twoFactorCode: steamGuardCode(account),
      rememberPassword: true,
    });

    const start = Date.now();
    while (!this.ready && Date.now() - start < timeoutMs) await sleep(250);
    if (!this.ready) throw new Error(`GC timeout for ${name}`);
  }

  inviteToParty(targetSteam64) {
    const target = normalizeSteam64(targetSteam64);
    const name = accountLogName(this.account);
    if (!target) throw new Error('empty target steam64');
    log(`[GC:${name}] inviteToParty target=${target}`);
    this.dota.sendPartial(GC_INVITE_TO_PARTY, { steamId: target });
  }

  acceptPartyInvite(partyId = null) {
    const name = accountLogName(this.account);
    const pid = partyId || this.lastPartyId || (this.invites.length ? this.invites[this.invites.length - 1].partyId : null);
    if (!pid) {
      log(`[GC:${name}] no party_id known yet; cannot accept. Waiting may be needed.`);
      return false;
    }
    log(`[GC:${name}] accept party_id=${pid}`);
    this.dota.sendPartial(GC_PARTY_INVITE_RESPONSE, { partyId: pid, accept: true });
    return true;
  }

  async close() {
    try { this.client.gamesPlayed([]); } catch (_) {}
    try { this.client.logOff(); } catch (_) {}
    await sleep(300);
  }
}

function requiredSteam64s(plan, action) {
  const set = new Set();
  for (const party of plan.parties || []) {
    if (action === 'invite' || action === 'full') set.add(normalizeSteam64(party.leader && party.leader.steam64));
    if (action === 'accept' || action === 'full') for (const bot of (party.bots || [])) set.add(normalizeSteam64(bot.steam64));
  }
  set.delete('');
  return Array.from(set);
}

async function main() {
  const action = arg('action', 'full');
  const accountsFile = path.resolve(arg('accounts', path.join(__dirname, '..', '..', 'data', 'config', 'gc_accounts.json')));
  const planFile = path.resolve(arg('plan', path.join(__dirname, '..', '..', 'data', 'config', 'gc_party_plan.json')));
  const deleteAccountsAfterRead = argBool('delete-accounts-after-read', false);
  const { cfg, map } = loadAccountMap(accountsFile);
  if (deleteAccountsAfterRead || cfg.deleteAfterRead) {
    try { fs.unlinkSync(accountsFile); log(`[GC] deleted runtime accounts file after read: ${accountsFile}`); }
    catch (err) { log(`[GC] could not delete runtime accounts file: ${err.message || err}`); }
  }
  const plan = readJson(planFile);
  const options = Object.assign({ inviteDelayMs: 1500, acceptWindowMs: 90000 }, cfg.options || {});

  log(`[GC] action=${action} accounts=${accountsFile} plan=${planFile}`);
  const sessions = new Map();
  for (const steam64 of requiredSteam64s(plan, action)) {
    const account = map.get(steam64);
    if (!account) fail(`[GC] Missing credentials for steam64=${steam64} in ${accountsFile}`);
    const session = new DotaGcSession(Object.assign({}, account, { steam64 }), 'auto');
    sessions.set(steam64, session);
  }

  for (const session of sessions.values()) await session.connect(options.connectTimeoutMs || 90000);

  if (action === 'invite' || action === 'full') {
    for (const party of plan.parties || []) {
      const leaderSteam64 = normalizeSteam64(party.leader && party.leader.steam64);
      const leader = sessions.get(leaderSteam64);
      if (!leader) continue;
      for (const bot of (party.bots || [])) {
        leader.inviteToParty(bot.steam64);
        await sleep(options.inviteDelayMs || 1500);
      }
    }
  }

  if (action === 'accept' || action === 'full') {
    const start = Date.now();
    while (Date.now() - start < (options.acceptWindowMs || 90000)) {
      let acceptedAny = false;
      for (const party of plan.parties || []) {
        for (const bot of (party.bots || [])) {
          const session = sessions.get(normalizeSteam64(bot.steam64));
          if (!session) continue;
          if (session.acceptPartyInvite()) acceptedAny = true;
        }
      }
      if (acceptedAny) break;
      await sleep(1000);
    }
  }

  await sleep(2000);
  for (const session of sessions.values()) await session.close();
  log('[GC] done');
}

main().catch(err => fail(err.stack || err.message || String(err)));
