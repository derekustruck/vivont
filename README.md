# Vivont

A native Unreal Engine runtime for conversational digital humans.

https://github.com/user-attachments/assets/d4ad6040-8e93-436f-8783-535b5d11193a

Vivont is an experimental Unreal Engine plugin built around a simple question: how much of an intelligent digital character can run entirely inside the engine?

Right now it does real-time audio-to-blendshape facial animation through Unreal's Neural Network Engine (NNE), with hosted LLM and TTS providers handling conversation and speech. The long-term goal is a character that runs fully local without cloud dependencies, per-token costs, and no internet required.

## Why

Most AI characters today are stitched together from hosted services: one for language, one for speech, one for facial animation, plus something to orchestrate it all. The quality is genuinely good, but you pay for it in recurring API costs, latency, privacy trade-offs, and a hard dependency on a network connection.

Vivont starts from a different premise: facial animation should behave like any other Unreal subsystem. The runtime is built on native engine execution, local ONNX inference through NNE, MetaHuman LiveLink, and as few external dependencies as possible. The LLM and TTS providers are implementation details, not architectural requirements. The prototype uses hosted providers today for LLM and TTS because they're the best quality-to-effort trade-off, but each one sits behind a modular interface and can be swapped for a local model as those mature.

The point isn't to get rid of cloud services on principle. It's to make sure the architecture can move toward fully local execution as local models get good enough.

## Current status

This is an active research project. The prototype currently supports:

- Real-time conversational interaction
- Modular LLM and TTS integration
- Local audio-to-blendshape inference (ONNX via Unreal NNE)
- LiveLink facial animation driving MetaHumans

The public repo contains the Unreal runtime and plugin source. Model weights, datasets, training infrastructure, and research tooling stay private while development continues.

## Architecture

```
User
   │
   ▼
Conversation Provider
(OpenAI)
   │
   ▼
Speech Provider
(OpenAI realtime and ElevenLabs API)
   │
   ▼
24 kHz PCM Audio
   │
   ▼
Vivont Audio→Blendshape Model
   │
   ▼
Unreal Neural Network Engine
   │
   ▼
LiveLink
   │
   ▼
MetaHuman
```

Only the audio-to-blendshape runtime is Vivont-specific. Everything else is a replaceable component.

## Training data

The current model was trained on **LESS THAN AN HOUR** of facial performance capture, and it already produces convincing real-time conversational animation. Which shows the robustness of the model architecture.

Next steps on the model side: a bigger capture corpus, the complete facial control set, better emotional range, more temporal stability, and support for multiple speakers and voices. The target is a production-quality model that drives the full facial rig in real time, entirely in-engine.

## How this relates to NVIDIA Audio2Face

NVIDIA's [Audio2Face](https://github.com/NVIDIA/Audio2Face-3D) is a production-ready platform for AI-driven facial animation — open models, training frameworks, SDKs, an Unreal integration, and deployment options from local execution to cloud services. Vivont isn't trying to compete with that.

The difference is the question being asked. Audio2Face is a comprehensive digital human ecosystem; Vivont asks how small and self-contained a conversational character can get when every subsystem is designed to live inside Unreal Engine.

| NVIDIA Audio2Face | Vivont |
|-------------------|---------|
| Comprehensive digital human ecosystem | Focused Unreal Engine research project |
| Production-ready authoring and deployment tools | Experimental runtime architecture |
| Multiple deployment models (local SDKs, Unreal plugin, NIM services) | Native Unreal Engine runtime |
| Open models and training framework | Private research model (for now) |
| Local and remote inference | Local, in-engine execution |

Both projects may well end up in similar places. Vivont's contribution is working out what that looks like from an Unreal-first perspective.

## Roadmap

Done:

- Native Unreal Engine runtime
- Local ONNX inference
- Unreal NNE integration
- MetaHuman LiveLink
- Real-time conversational prototype

Next:

- Full facial control support
- Larger training corpus
- Streaming inference (currently captured and chunked)
- Emotion presentation layer
- Persistent character memory
- Local LLM integration
- Local text-to-speech
- Cross-platform deployment
- Production-ready plugin packaging

## What's in this repo

Included: the Unreal Engine plugin source, runtime orchestration, the ONNX/NNE inference pipeline, LiveLink integration, docs, and demo media.

Not included: production model weights, training datasets, the dataset preparation pipeline, the training framework, and research tooling. Those stay private while the research is active.

## License

See the LICENSE file.
