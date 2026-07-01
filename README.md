# Vivont

[Vivont alpha demo](Videos/VivontAlpha.mp4)

Vivont is an Unreal Engine 5.7 plugin for a real-time conversational MetaHuman avatar. A user sends text, an LLM generates a reply, text-to-speech returns voice audio, and Vivont drives the character face through LiveLink.

This public repository contains the Unreal plugin/runtime code. It does not include the private Vivont model, training data, dataset preparation pipeline, training suite, checkpoints, or model export workflow.

## Public Repository Scope

Included:

- Unreal Engine plugin source under `Plugins/VivontPlugin/`
- Runtime orchestration for text input, LLM response handling, TTS audio playback, local inference handoff, and LiveLink curve publishing
- In-engine ONNX/NNE runner code that can load a compatible model package
- Plugin UI/content assets that are safe to ship publicly
- Public docs, TODOs, license, and demo media

Not included:

- Vivont production ONNX model weights
- Private model manifests and export artifacts beyond any safe public placeholders
- Training or dataset preparation code
- Training datasets, validation sets, checkpoints, logs, or experiment history
- Internal research notes and archived development docs

The facial model remains private. Public users should treat the model loader as an integration point for a compatible local ONNX package, not as a bundled model release.

## Runtime Architecture

The current runtime path is:

1. `UVivontWidget` accepts text input from the in-engine UI.
2. `USimpleOpenAIClient` sends the prompt to an LLM and sends the response text to TTS.
3. TTS returns 24 kHz mono PCM speech audio.
4. `UVivontAnimationManager` stores the audio and submits it to `UVivontBlendshapeRunner`.
5. `UVivontBlendshapeRunner` loads a local ONNX model package through Unreal NNE.
6. `FVivontLiveLinkSource` publishes model-defined `CTRL_expressions_*` curves to the MetaHuman face.
7. `UVivontAudioSubsystem` plays the speech audio so voice and facial animation stay synchronized.

The important design point is that facial inference is intended to run locally inside Unreal, not through a hosted Python server. The runtime uses Unreal NNE, preferring `NNERuntimeORTDml` for local GPU/DirectML inference with CPU fallback where available.

## Private Model Contract

Vivont's internal model package is private, but the runtime integration contract is manifest-driven. A compatible package is expected to provide:

- An ONNX model file
- A JSON manifest that describes the model file, audio sample rate, input window length, output frame rate, output curve names, and normalization stats
- MetaHuman-compatible control-rig curve names in model output order

By default, the plugin looks for a manifest at:

```text
Plugins/VivontPlugin/Content/Models/vivont_manifest.json
```

The manifest points to the ONNX file relative to its own directory. If no compatible private model package is present, the plugin can compile, but facial inference will not run.

## Public Setup Notes

To use the public plugin in a host Unreal project:

1. Use Unreal Engine 5.7.
2. Enable `NNERuntimeORT`, LiveLink, and the MetaHuman-related plugins required by your character setup.
3. Place `Plugins/VivontPlugin/` in the host project's `Plugins/` directory.
4. Provide your own compatible local model package, or obtain the private Vivont model package through the appropriate private distribution path.
5. Configure an OpenAI API key, or replace `USimpleOpenAIClient` with your own LLM/TTS provider.
6. Connect the LiveLink subject to a MetaHuman face rig that consumes the manifest's `CTRL_expressions_*` curves.

Do not commit API keys. Use Unreal editor settings, local untracked config, environment variables, or another secret-management path appropriate for your project.

## Repository Layout

- `Plugins/VivontPlugin/` - public Unreal plugin source and safe plugin content
- `Videos/` - public demo media
- `docs/ProductDefinition.md` - public product definition
- `TODO.md` - public execution list
- `README.md` - this public system overview
- `LICENSE` - repository license

Internal training, prep, data, model, and research archives are intentionally omitted from the public repository.

