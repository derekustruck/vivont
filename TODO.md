# Vivont TODO

Derived from `docs/ProductDefinition.md` and the current `Plugins/VivontPlugin` implementation.

## In-Engine Work

- [ ] Add a listening animation state that starts immediately when the user begins input.
- [ ] Build a local array of listening animation options and expose selection rules in editor assets.
- [ ] Add contextual thinking animations and short thinking voice lines such as "Hmm, good question. Let me check on that."
- [ ] Create an animation/effect trigger library for text events, including face animation, body animation, and Niagara effects.
- [ ] Add product-demo trigger examples such as birthday reactions, customer-service gags, co-watching reactions, and child-app guidance moments.
- [ ] Build an emotion presentation layer that maps conversation tone into facial pose, body pose, idle state, and transition animation choices.
- [ ] Add a character selector for demo use.
- [ ] Generalize animation blueprints so the plugin works across MetaHuman-compatible characters with minimal setup.
- [ ] Verify the current LiveLink subject and `CTRL_expressions_*` curve hookup on each target MetaHuman.
- [ ] Package required UI, animation, LiveLink, and model assets under `Plugins/VivontPlugin/Content/` so the plugin can drop into a clean host project.
- [ ] Run an end-to-end visual QA pass: type input, receive LLM text, play TTS, run local in-engine model, animate face, and sync audio.
- [ ] Record baseline latency numbers for text-to-audio, audio-to-first-frame, and full utterance playback.

## Coding Work

- [ ] Add a response-state controller for listening, thinking, speaking, reacting, and returning to idle.
- [ ] Implement trigger parsing for submitted user text and generated LLM text.
- [ ] Define a data-driven trigger schema for animation clips, voice lines, Niagara effects, priority, cooldowns, and character compatibility.
- [ ] Add an emotion mapper that converts LLM response tone into animation parameters and effect choices.
- [ ] Decide whether to expose the runtime as an actor component; if yes, add a component facade over the existing subsystems for easier host-project integration.
- [ ] Add streaming support for chat and/or TTS to reduce time-to-first-frame.
- [ ] Pipeline chunked inference so LiveLink playback can begin before the full utterance finishes generating.
- [ ] Log the active NNE backend at runtime and make GPU/DirectML fallback obvious in editor logs.
- [ ] Add per-utterance timing logs around TTS, model load, inference, LiveLink handoff, and audio playback.
- [ ] Validate multi-frame ONNX output quality in-engine and add overlap blending if block boundaries are visible.
- [ ] Keep the model manifest as the single runtime contract and prepare the export path for 40-to-68 curve expansion.
- [ ] Move long-lived runtime state from `UEngineSubsystem` to `UGameInstanceSubsystem` or the chosen actor-component boundary where that improves host-project behavior.
- [ ] Remove or quarantine unused legacy ARKit/68-curve code after confirming current MetaHuman curve paths no longer depend on it.
- [ ] Keep API keys out of tracked config and document the local secret-loading path.
- [ ] Add a repeatable build or smoke check for `Plugins/VivontPlugin`.
- [ ] Migrate to ElevenLabs for expressive TTS and use OpenAI for chat text generation only.

