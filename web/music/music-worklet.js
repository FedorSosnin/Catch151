// The soundtrack page's audio: the game's own music player (music.wasm, host/music_web.cpp) run on the audio thread,
// a block of 128 samples at a time, so a song loops forever the way it does in the game. It tells the page which song
// is playing and, about 30 times a second, what each of its four voices just played (for the page's scopes).
const SCOPE = 1024;   // samples of each voice the page draws
const BLOCK = 128;

class Catch151Music extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.e = null;
    this.want = -1;
    this.playing = -1;
    this.frames = 0;                                  // samples since the song playing began
    this.scope = new Float32Array(4 * SCOPE);         // each voice's last SCOPE samples, a ring
    this.at = 0;
    this.sinceScope = 0;
    this.port.onmessage = ({ data }) => {
      if (data.type === 'play') {
        this.want = data.track;
        if (this.e) this.e.mwPlay(data.track);
      }
    };
    WebAssembly.instantiate(options.processorOptions.wasm, {}).then(({ instance }) => {
      const e = instance.exports;
      e._initialize();
      e.mwBegin(sampleRate);
      this.voices = [0, 1, 2, 3].map((c) => new Float32Array(e.memory.buffer, e.mwVoice(c), BLOCK));
      const dec = (p) => { const m = new Uint8Array(e.memory.buffer); let s = ''; while (m[p]) s += String.fromCharCode(m[p++]); return s; };
      const songs = [];
      for (let i = 0; i < e.mwCount(); i++) {
        songs.push({ name: dec(e.mwName(i)), bpm: e.mwBpm(i), loops: e.mwLoops(i), intro: e.mwIntroTicks(i), loop: e.mwLoopTicks(i) });
      }
      this.e = e;
      if (this.want >= 0) e.mwPlay(this.want);
      this.port.postMessage({ type: 'ready', songs, rate: sampleRate });
    });
  }

  process(inputs, outputs) {
    const out = outputs[0];
    if (!this.e || !out.length) return true;
    const n = out[0].length;
    this.e.mwRender(n);
    const left = out[0];
    left.fill(0);
    for (let c = 0; c < 4; c++) {
      const v = this.voices[c];
      const ring = c * SCOPE;
      for (let i = 0; i < n; i++) {
        left[i] += v[i];
        this.scope[ring + ((this.at + i) % SCOPE)] = v[i];
      }
    }
    this.at = (this.at + n) % SCOPE;
    for (let ch = 1; ch < out.length; ch++) out[ch].set(left);
    const p = this.e.mwPlaying();
    if (p !== this.playing) {
      this.playing = p;
      this.frames = 0;
      this.port.postMessage({ type: 'track', track: p });
    } else {
      this.frames += n;
    }
    this.sinceScope += n;
    if (this.sinceScope >= sampleRate / 30) {
      this.sinceScope = 0;
      const voices = new Float32Array(4 * SCOPE);  // oldest first
      for (let c = 0; c < 4; c++) {
        const ring = c * SCOPE;
        voices.set(this.scope.subarray(ring + this.at, ring + SCOPE), c * SCOPE);
        voices.set(this.scope.subarray(ring, ring + this.at), c * SCOPE + SCOPE - this.at);
      }
      this.port.postMessage({ type: 'scope', voices, frames: this.frames }, [voices.buffer]);
    }
    return true;
  }
}

registerProcessor('catch151-music', Catch151Music);
