# Buddy relay

Browsers can't send the Knomi's local Wi-Fi messages (UDP broadcasts on port 47115), so Catch 151 in a browser meets its buddies through this relay: a Cloudflare Worker with one Durable Object that passes the same 65-byte buddy messages (`buddy.h`) between devices on the same home network.

- **Grouping:** devices are grouped by public address: their IPv4, or their IPv6 /64 plus the IPv4 they report (computers and phones often connect over IPv6, Knomis over IPv4).
- **What it passes:** only well-formed buddy messages, at most 8 a second per device, 16 devices per group. It logs nothing, and every check on the messages stays on the devices.
- **What it keeps:** the leaderboard (`src/board.js`): each minigame's best ten, with the gamer tag each came with and the day, and the heaviest fish anyone has landed in Reel Fishing (its weight, its kind and if it was shiny). One entry a device, kept by a hash of its id; nothing about who or where. Each device sends its own bests (`BM_SCORE` messages, and `BM_FISH` for its heaviest fish, to the relay alone and never passed on) when it connects, at each game's end, and when its gamer tag changes. A score past what its game can reach is ignored, as is a weight its kind can't reach (over 1.2 times its Pokédex weight) or a Pokémon that isn't a Beach fish.
- **The first of a game:** when a device's best is the first of its game's ten, the relay answers on its connection with a `BM_SCORE` of its own (from `0xFFFFFFFF`, to that device): the game gives it the High Score medal, the golden cartridge, once. Copies older than the leaderboard drop it.
- **Gamer tags:** up to four characters. A tag on the rude-word list (the same as the game's, `src/gamertag.h`, read through stand-ins like 5 for S) shows as `****`; the list here can grow without an update to the game. `node --test relay/test/*.test.mjs` checks the board's rules, and that both lists match.
- **Cost:** it tells each device how many others share its group (`{"n": 2}`), and a device that is alone sends nothing. A `ping` text keeps quiet connections open and is answered without waking the object. Cloudflare's free plan covers it; if it ever runs out, only relay buddies pause until the next day, and the games keep working.

- **Live count:** [`/stats`](https://catch151-relay.fedor-dd0.workers.dev/stats) says how many devices are connected right now, all kinds together: `{"online": 12}`. That's every Knomi on Wi-Fi, Mac and Windows app and open browser tab, so it counts players who are playing now, not everyone who has the game. It's only the number, refreshed at most once a minute.
- **Biggest fish:** `/stats` also has the heaviest fish, in pounds as the game shows it: `{"online": 12, "fish": {"lb": 308.6, "kind": "Cloyster", "shiny": false, "day": "2026-11-16"}, "biggest": "308.6 lb Cloyster"}` (`"biggest": "none yet"` until the first). The README's **biggest fish** badge shows `biggest`.

- **Leaderboard:** `/stats` has it too, as `"board": {"games": [{"game": 6, "name": "Pinball", "top": [{"tag": "FED", "score": 48250, "shown": "48,250", "day": "2026-11-16"}, ...]}, ...], "fish": [{"tag": "ZOE", "lb": 551.2, "kind": "Gyarados", "shiny": false, "day": "2026-11-16"}, ...]}`, and as a picture, [`/board.svg`](https://catch151-relay.fedor-dd0.workers.dev/board.svg): each game's best in the game's own font with its cartridge (`src/board_art.js`, made by `tools/relay_board_art.sh`), kept five minutes. The README shows the picture, and the website's [scores page](https://fedorsosnin.github.io/Catch151/leaderboard/) the top tens.

The fish and the board need the `Lobby` class's code, which runs in `pokeknomigochi-relay`: deploy both (`wrangler deploy`, then `wrangler deploy -c wrangler-catch151.toml`) before a release whose copies send them.

Deploy with [Wrangler](https://developers.cloudflare.com/workers/wrangler/): `cd relay && wrangler deploy`.

**Two addresses, one lobby (since 2.2.0, the rename to Catch 151):** `pokeknomigochi-relay` (`wrangler.toml`) holds the
lobby, and the copies up to 2.1.4 connect to it. `catch151-relay` (`wrangler-catch151.toml`, deploy with
`wrangler deploy -c wrangler-catch151.toml`) runs the same code (`src/catch151.js`) and reaches the same lobby through
its `LOBBY` binding's `script_name`, so copies on either address meet. Never rename `wrangler.toml`'s Worker: that
would start a second, empty lobby.
