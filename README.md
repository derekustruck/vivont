# Vivont

> **A native Unreal Engine runtime for conversational digital humans.**

https://github.com/user-attachments/assets/d4ad6040-8e93-436f-8783-535b5d11193a

Vivont is an experimental Unreal Engine plugin exploring a simple question:

> **What if an intelligent digital character could live entirely inside the game engine?**

Rather than treating facial animation as an external service, Vivont is designed around a native runtime where conversation, speech, animation, and eventually intelligence all execute as modular subsystems inside Unreal Engine.

Today, the project demonstrates real-time audio-to-blendshape facial animation using Unreal's Neural Network Engine (NNE). Tomorrow, we envision a fully local conversational character that requires no cloud infrastructure to exist.

---

# Vision

AI-powered characters are becoming increasingly capable, but most still rely on multiple hosted services for language generation, speech synthesis, facial animation, and orchestration.

Those services provide exceptional quality today, but they also introduce recurring API costs, latency, privacy considerations, and an internet dependency.

Vivont explores a different future.

We believe conversational characters should eventually become native software rather than cloud services.

As language models, speech synthesis, and consumer hardware continue to improve, we envision a runtime where:

- Language models execute locally.
- Speech synthesis runs locally.
- Facial animation is generated locally.
- Character memory persists between sessions.
- Characters remain available without an internet connection.
- No per-message or per-token API costs are required.

The current prototype intentionally uses hosted LLM and text-to-speech providers because they offer the best balance of quality and development speed today. Both systems are modular by design and are intended to be replaceable as local alternatives mature.

Our goal is not to eliminate cloud services—it is to ensure Vivont can naturally evolve toward a fully local architecture as the technology becomes practical.

---

# Why Vivont?

Vivont is built around one architectural philosophy:

> **Facial animation should behave like any other Unreal Engine subsystem.**

The current runtime is intentionally designed around:

- Native Unreal Engine execution
- Unreal Neural Network Engine (NNE)
- Local ONNX inference
- MetaHuman LiveLink
- Modular AI providers
- Minimal runtime dependencies

Large language models and text-to-speech providers are implementation details—not architectural requirements.

As better local models emerge, they can replace today's hosted services without changing the rest of the runtime.

---

# Current Status

Vivont is an active research project.

The current prototype supports:

- Real-time conversational interaction
- Modular LLM integration
- Modular text-to-speech integration
- Local audio-to-blendshape inference
- Unreal NNE execution
- LiveLink facial animation
- MetaHuman integration

The public repository contains the Unreal Engine runtime and plugin source.

Model weights, datasets, training infrastructure, and research tooling remain private while development continues.

---

# A Small Dataset, Surprisingly Good Results

One of the most interesting aspects of Vivont is the amount of training data used.

The current prototype was trained on **less than one hour of facial performance capture** and already produces convincing real-time conversational animation.

Future work will focus on:

- expanding the capture corpus
- supporting the complete facial control set
- improving emotional expressiveness
- increasing temporal stability
- supporting multiple speakers and voices

Our long-term objective is a production-quality model capable of driving the full facial rig while maintaining real-time performance entirely inside Unreal Engine.

---

# Runtime Architecture

```
User
   │
   ▼
Conversation Provider
(OpenAI today, local tomorrow)
   │
   ▼
Speech Provider
(ElevenLabs today, local tomorrow)
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

Every stage is intentionally modular.

Only the audio-to-blendshape runtime is Vivont-specific. Conversation providers, speech providers, and future local models are interchangeable components.

---

# Relationship to NVIDIA Audio2Face

NVIDIA's Audio2Face ecosystem represents an impressive production-ready platform for AI-driven facial animation. It includes open models, training frameworks, SDKs, Unreal Engine integration, and deployment options ranging from local execution to scalable cloud services.  [oai_citation:0‡GitHub](https://github.com/NVIDIA/Audio2Face-3D?utm_source=chatgpt.com)

Vivont is not intended to replace that ecosystem.

Instead, it explores a different architectural question:

> **How small and self-contained can a conversational character become when every subsystem is designed to live inside Unreal Engine?**

The distinction is one of architectural philosophy rather than capability.

| NVIDIA Audio2Face | Vivont |
|-------------------|---------|
| Comprehensive digital human ecosystem | Focused Unreal Engine research project |
| Production-ready authoring and deployment tools | Experimental runtime architecture |
| Multiple deployment models (local SDKs, Unreal plugin, NIM services) | Native Unreal Engine runtime |
| Open models and training framework | Private research model (currently) |
| Supports local and remote inference | Designed around local in-engine execution |

As both projects evolve, they may ultimately converge toward many of the same goals. Vivont's contribution is exploring what that future looks like from an Unreal-first perspective.

---

# Roadmap

Current milestones:

- ✅ Native Unreal Engine runtime
- ✅ Local ONNX inference
- ✅ Unreal NNE integration
- ✅ MetaHuman LiveLink
- ✅ Real-time conversational prototype

Next milestones:

- ⬜ Full facial control support
- ⬜ Larger training corpus
- ⬜ Streaming inference
- ⬜ Emotion presentation layer
- ⬜ Persistent character memory
- ⬜ Local LLM integration
- ⬜ Local text-to-speech
- ⬜ Cross-platform deployment
- ⬜ Production-ready plugin packaging

---

# Repository

This repository contains the Unreal Engine runtime and integration layer.

Included:

- Unreal Engine plugin source
- Runtime orchestration
- Local ONNX/NNE inference pipeline
- LiveLink integration
- Public documentation
- Demo media

Not included:

- Production model weights
- Training datasets
- Dataset preparation pipeline
- Training framework
- Experiment history
- Research tooling

The repository is intended to demonstrate the runtime architecture while protecting active research assets during development.

---

# License

See the accompanying LICENSE file.
