#pragma once
// The game's Pokedex, moves and type chart as JSON for the Catch 151 Dex page (tools/make_sprite_gallery.py puts it
// in; tools/sprite_gallery.html shows it), straight from the tables and rules the game itself uses, so the page can't
// say something the game doesn't:
//   types   TYPE_NAME (dexfacts.h), in TY_* order
//   chart   btlEffect() (battle.h), a row per attacking type, a digit per defending type: x2, so 0 = no effect,
//           1 = half, 2 = normal, 4 = double
//   moves   MOVE_TBL (moves.h): [name, type, power], power as the game counts it (Red/Blue's, with accuracy, hits in
//           a row and charging turns folded in)
//   mons    one line per Pokemon (dex.h, dexfacts.h, moves.h, battle.h):
//             type, [hp, atk, def, spe] (its base stats), evo ([[into, level], ...]; Eevee's three branches),
//             rarity (R_*: 0 an evolved form, 1 common, 2 rare, 3 legendary), zone (ZONE_OF: where a Search finds
//             it), min (the level your Pokemon must be for it to turn up: btlMinLevel()), fact (DEX_FACT: what its
//             Pokedex page says), learn ([[level, move], ...]: LEARN_TBL), battle ([[from level, move, move], ...]:
//             btlMovesFor(), the two moves on its battle buttons from that level on, one when it has a single move;
//             empty for Ditto, which copies its foe's)
//   zones    where they're caught (zones.h): names, the five zones in Z_* order; legends, ZONE_LEGEND (each zone's
//           legend, turning up on legendPermil Searches in 1000 there, from the first); starters, the four you can
//           start with (starter_v2.h's V2_STARTERS). Everyone else is caught
//   catch    the chances of each catch (Pet::rollStars(), WILD_SHINY_PCT): shiny, its % of the wild ones; stars, the %
//           of catches with 1, 2 and 3 stars; searchLevel (each zone's) and wildLevel, the level from which every one
//           turns up on a Search there or in a Wild Encounter (dexOddsLevel()); legend, [the level, the Pokemon you
//           have] from which a legend can turn up in a Wild Encounter
//           and per Pokemon (mons): search, how often a Search in its zone turns it up, and wild, a Wild Encounter
//           (dexSearchOdds(), dexWildOdds())
//   training how your Pokemon is trained (pet.h, pet.cpp): each kind up to max; win, ATK a battle won; careHours,
//           hours of good care for 1 DEF, and graceMin, the minutes awake a need may dip under 40; levelHours of
//           age a level, battles counting up to levelCap; genes, [lowest, top of 1 star, top of 2, highest] (%);
//           games, what each mini game gives SPD: [name, its unit, per that many, this much]
// `make -C host dex` writes tools/dex.json; the host tests check that file is what this makes, and the pet's tests
// (test/test_pet.cpp) that DEX_TRAIN_GAMES is what the games really give.
#include <math.h>
#include <stdio.h>
#include <string>
#include "../src/pet.h"
#include "../src/dex.h"
#include "../src/moves.h"
#include "../src/battle.h"
#include "../src/dexfacts.h"
#include "../src/zones.h"
#include "../src/minigames.h"

// what each mini game trains (SPD), as the page says it: Pet::playResult(), runResult(), surfResult(), dungeonResult()
enum DexTrainKind : uint8_t { DEX_TRAIN_BERRY, DEX_TRAIN_RUN, DEX_TRAIN_SURF, DEX_TRAIN_DUNGEON };
struct DexTrainGame { DexTrainKind kind; const char *name, *unit; uint8_t per, gain; };
static const DexTrainGame DEX_TRAIN_GAMES[] = {
  { DEX_TRAIN_BERRY, "Berry Drop", "points", 5, 1 },
  { DEX_TRAIN_RUN, "Rock Hopper", "seconds", 20, 1 },
  { DEX_TRAIN_SURF, "PikaSurf", "seconds", 20, 1 },
  { DEX_TRAIN_DUNGEON, "Dungeon Crawler", "floors", 5, 2 },
};
#define DEX_TRAIN_MAX 100   // each kind of training stops here (Pet::clamp100)
#define DEX_GENE_MIN 90     // a Pokemon's genes, % of its base stats (Pet::hatch())
#define DEX_GENE_MAX 110
// the four you can start with (starter_v2.h's V2_STARTERS: test_tools.py checks they agree) and the zones' names, as
// the game's English shows them (S_ZONE_FOREST...)
static const int16_t DEX_STARTERS[] = { 1, 4, 7, 25 };
static const char *const DEX_ZONE_NAMES[ZONE_COUNT] = { "Forest", "Beach", "Cave", "Mansion", "Field" };

// How often it turns up (the page's Catching tab): the chance a Search in its zone finds it (Pet::pickZoneSpecies()),
// and a Wild Encounter (Pet::pickWildSpecies()), once yours is strong enough for every one of them (dexOddsLevel())
// and you have them all, so neither leans toward the missing ones; with no Repel. test_pet.cpp checks them against
// the two pickers.
static inline double dexSearchOdds(int16_t d) {
  uint8_t z = zoneOf(d);
  if (d == ZONE_LEGEND[z]) return ZONE_LEGEND_PERMIL / 1000.0;  // on any Search
  int total = 0;  // the rest by weight
  for (int16_t s = 1; s <= DEX_COUNT; s++)
    if (zoneOf(s) == z && s != ZONE_LEGEND[z]) total += zoneWeight(s);
  return (1 - ZONE_LEGEND_PERMIL / 1000.0) * zoneWeight(d) / total;
}
static inline double dexWildOdds(int16_t d) {
  const double look = WILD_NEW_PCT / 100.0, legend = WILD_LEGEND_PCT / 100.0, sits = WILD_RARE_SKIP / 100.0;
  int legends = 0, rares = 0, others = 0;
  for (int16_t s = 1; s <= DEX_COUNT; s++) {
    uint8_t r = DEX_TBL[s].rarity;
    (r == R_LEGENDARY ? legends : r == R_RARE ? rares : others)++;
  }
  // a look among the missing ones finds none (you have them all), and so does a legend's turn then: everyone
  // else as likely. Otherwise a legend's turn is the legends'; or everyone else, each rare one sitting out
  // `sits` of the time
  if (DEX_TBL[d].rarity == R_LEGENDARY) return (1 - look) * legend / legends;
  bool rare = DEX_TBL[d].rarity == R_RARE;
  int with = others + (rare ? 1 : 0), n = rares - (rare ? 1 : 0);  // it, the others always there, n that may sit out
  double e = 0, c = 1;  // E[1 / (with + those of the n that stay)], c the binomial coefficient
  for (int k = 0; k <= n; k++) {
    e += c * pow(1 - sits, k) * pow(sits, n - k) / (with + k);
    c = c * (n - k) / (k + 1);
  }
  return look / (rares + others) + (1 - look) * (1 - legend) * (rare ? 1 - sits : 1) * e;
}
// the level from which every one turns up: a wild one is up to 3 below yours (btlWildLevel()); a Wild Encounter's
// legend needs WILD_LEGEND_LVL. zone: 0-4, or -1 for a Wild Encounter
static inline int dexOddsLevel(int zone) {
  int top = zone < 0 ? WILD_LEGEND_LVL : 1;
  for (int16_t s = 1; s <= DEX_COUNT; s++)
    if (zone < 0 ? DEX_TBL[s].rarity != R_LEGENDARY : (zoneOf(s) == zone && s != ZONE_LEGEND[zone]))
      top = btlMinLevel(s) > top ? btlMinLevel(s) : top;
  return top + 3;
}

static inline std::string dexJsonStr(const char *s) {
  std::string o = "\"";
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') o += '\\';
    o += *s;
  }
  return o + "\"";
}

static inline std::string dexPageJson() {
  std::string out = "{\n\"types\":[";
  char b[200];
  for (int t = 0; t < TY_COUNT; t++) out += (t ? "," : "") + dexJsonStr(TYPE_NAME[t]);
  out += "],\n\"chart\":[";
  for (int a = 0; a < TY_COUNT; a++) {
    out += a ? ",\"" : "\"";
    for (int d = 0; d < TY_COUNT; d++) out += (char)('0' + btlEffect(a, d));
    out += "\"";
  }
  out += "],\n\"moves\":[";
  for (size_t i = 0; i < sizeof(MOVE_TBL) / sizeof(MOVE_TBL[0]); i++) {
    snprintf(b, sizeof(b), "%s\n[%s,%d,%d]", i ? "," : "", dexJsonStr(MOVE_TBL[i].name).c_str(), MOVE_TBL[i].type,
             MOVE_TBL[i].power);
    out += b;
  }
  snprintf(b, sizeof(b), "\n],\n\"training\":{\"max\":%d,\"win\":%d,\"careHours\":%d,\"graceMin\":%d,\"levelHours\":%d,"
           "\"levelCap\":%d,\"genes\":[%d,%d,%d,%d],\"games\":[", DEX_TRAIN_MAX, BTL_WIN_TRAIN, GOOD_CARE_MIN / 60,
           CARE_GRACE_MIN, MINUTES_PER_LEVEL / 60, BTL_LEVEL_CAP, DEX_GENE_MIN, GENE_BAND_1, GENE_BAND_2, DEX_GENE_MAX);
  out += b;
  for (size_t i = 0; i < sizeof(DEX_TRAIN_GAMES) / sizeof(DEX_TRAIN_GAMES[0]); i++) {
    const DexTrainGame &g = DEX_TRAIN_GAMES[i];
    snprintf(b, sizeof(b), "%s[%s,%s,%d,%d]", i ? "," : "", dexJsonStr(g.name).c_str(), dexJsonStr(g.unit).c_str(),
             g.per, g.gain);
    out += b;
  }
  out += "]},\n\"zones\":{\"names\":[";
  for (int z = 0; z < ZONE_COUNT; z++) out += (z ? "," : "") + dexJsonStr(DEX_ZONE_NAMES[z]);
  out += "],\"legends\":[";
  for (int z = 0; z < ZONE_COUNT; z++) out += (z ? "," : "") + std::to_string(ZONE_LEGEND[z]);
  snprintf(b, sizeof(b), "],\"legendPermil\":%d,\"starters\":[", ZONE_LEGEND_PERMIL);
  out += b;
  for (size_t i = 0; i < sizeof(DEX_STARTERS) / sizeof(DEX_STARTERS[0]); i++)
    out += (i ? "," : "") + std::to_string(DEX_STARTERS[i]);
  snprintf(b, sizeof(b), "]},\n\"catch\":{\"shiny\":%d,\"stars\":[%d,%d,%d],\"searchLevel\":[", WILD_SHINY_PCT,
           100 - STAR_PCT_2 - STAR_PCT_3, STAR_PCT_2, STAR_PCT_3);
  out += b;
  for (int z = 0; z < ZONE_COUNT; z++) out += (z ? "," : "") + std::to_string(dexOddsLevel(z));
  snprintf(b, sizeof(b), "],\"wildLevel\":%d,\"legend\":[%d,%d]", dexOddsLevel(-1), WILD_LEGEND_LVL, WILD_LEGEND_DEX);
  out += b;
  out += "},\n\"mons\":{";
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    const DexEntry &e = DEX_TBL[d];
    snprintf(b, sizeof(b), "%s\n\"%d\":{\"type\":%d,\"stats\":[%d,%d,%d,%d],\"evo\":[", d > 1 ? "," : "", d, e.type,
             e.bHp, e.bAtk, e.bDef, e.bSpe);
    out += b;
    if (d == DEX_EEVEE) {
      snprintf(b, sizeof(b), "[134,%d],[135,%d],[136,%d]", e.evolveLevel, e.evolveLevel, e.evolveLevel);
      out += b;
    } else if (e.evolvesTo) {
      snprintf(b, sizeof(b), "[%d,%d]", e.evolvesTo, e.evolveLevel);
      out += b;
    }
    snprintf(b, sizeof(b), "],\"rarity\":%d,\"zone\":%d,\"min\":%d,\"search\":%.6g,\"wild\":%.6g,\"fact\":", e.rarity,
             zoneOf(d), btlMinLevel(d), dexSearchOdds(d), dexWildOdds(d));
    out += b;
    out += dexJsonStr(DEX_FACT[d]);
    out += ",\"learn\":[";
    for (uint16_t i = LEARN_START[d]; i < LEARN_START[d + 1]; i++) {
      snprintf(b, sizeof(b), "%s[%d,%d]", i > LEARN_START[d] ? "," : "", LEARN_TBL[i].level, LEARN_TBL[i].move);
      out += b;
    }
    out += "],\"battle\":[";
    if (d != DEX_DITTO) {
      int had = -1;
      for (int lvl = 1; lvl <= BTL_LEVEL_CAP; lvl++) {
        BtlMoves m = btlMovesFor(d, (uint8_t)lvl, d);
        int key = m.ids[0] * 256 + m.ids[1];
        if (key == had) continue;
        if (m.ids[0] == m.ids[1]) snprintf(b, sizeof(b), "%s[%d,%d]", had >= 0 ? "," : "", lvl, m.ids[0]);
        else snprintf(b, sizeof(b), "%s[%d,%d,%d]", had >= 0 ? "," : "", lvl, m.ids[0], m.ids[1]);
        out += b;
        had = key;
      }
    }
    out += "]}";
  }
  return out + "\n}\n}\n";
}
