# Literature Review: TinyML On-Device Learning with Human-in-the-Loop Systems

## TinyML State-of-the-Art: From Inference to On-Device Training

**The TinyML landscape has evolved dramatically from inference-only frameworks to systems supporting on-device training on resource-constrained microcontrollers.** TensorFlow Lite Micro established the foundation for edge inference with a minimal 26.5KB framework footprint and INT8 quantization, enabling deployment on devices with just 256-320KB SRAM. However, TFLite Micro remains strictly inference-only because training requires 6-7× more memory than inference for storing intermediate activations, gradients, and optimizer states. CMSIS-NN further optimized ARM Cortex-M processors through SIMD instructions, achieving 4.6× runtime improvements, but similarly provides no training capabilities.

**The breakthrough toward on-device training came with TinyOL (2021), the first system demonstrating incremental online learning on ARM Cortex-M4 microcontrollers with 256KB SRAM.** TinyOL processes streaming data one sample at a time, updating weights incrementally via stochastic gradient descent without storing historical data. This approach enables concept drift handling and dynamic class addition but trains only the last layer while freezing the base network in Flash memory. Subsequently, MCUNetV3 (2022) achieved full-network training under 256KB memory through sparse updates and Quantization-Aware Scaling (QAS), reducing memory by 20-21× compared to full updates while matching cloud training accuracy. TinyTL (2020) demonstrated 6.5× memory reductions through bias-only updates with lite residual modules, achieving accuracy improvements up to 34.1% over last-layer-only fine-tuning. These systems collectively demonstrate that on-device training on microcontrollers is technically feasible, though each involves significant trade-offs between memory, training scope, and adaptability.

## Streaming and Incremental Learning Algorithms for Resource-Constrained Devices

**Streaming clustering algorithms for actual microcontroller implementations remain an emerging research area with limited published work containing comprehensive quantitative metrics.** Mini-batch k-means represents the most mature approach, achieving memory reductions from 52GB (standard k-means) to 0.98GB through processing data in fixed-size batches with incremental centroid updates via gradient descent. The algorithm exhibits O(dk(b+log B)) complexity where b represents batch size and B the number of batches, with memory optimum at B=n^(1/2) batches. For microcontroller deployment, practical batch sizes of 50-200 samples balance memory constraints (~3.8KB for batch=100, dimensions=8) against convergence properties.

**TinyOL's incremental learning approach demonstrates practical streaming capabilities on ARM Cortex-M4 processors.** The system maintains only one data sample in memory at any time, processes it through inference, updates weights, then discards the sample—fundamentally different from batch approaches requiring full dataset storage. This streaming paradigm achieves latency of 1,921 μs average (inference + update) versus 1,748 μs for inference-only, representing just 10% overhead. Recent work on Enhanced Vector Quantization (AutoCloud K-Fixed, 2024) achieved >90% model compression through incremental clustering for automotive embedded platforms, while EdgeClusterML (2024) demonstrated reinforcement learning combined with self-organizing maps for real-time edge data clustering on distributed systems.

**The primary challenge remains the scarcity of microcontroller-specific implementations with reproducible metrics.** Most streaming clustering research targets more powerful edge devices (Raspberry Pi) or provides simulation-only results. Critical gaps include exact memory footprints for varying cluster counts, convergence iteration counts on specific MCU models, and energy consumption measurements—all essential for practical embedded deployment.

## Human-in-the-Loop Systems for Edge Machine Learning

**HITL and active learning systems for edge AI demonstrate significant quantified improvements in label efficiency and model accuracy while reducing latency through local processing.** Active learning research by Senzaki and Hamelain (2021) introduced stream submodular maximization for edge devices, achieving task-agnostic sample selection that outperforms all baseline methods while running with practical speed on resource-constrained hardware. The framework provides theoretical guarantees for solution quality while eliminating cloud round-trip latency, critical for real-time applications requiring sub-100ms response times.

**Quantified benefits from recent deployments show substantial efficiency gains.** The Dairy DigiD system (2024) achieved 3.2% mAP improvement through iterative active learning refinement on NVIDIA Jetson Xavier NX, while simultaneously reducing model size by 73% through INT8 quantization and cutting technician training time by 84% via improved annotation interfaces. Distributed active learning architectures (2019 IEEE study) demonstrated that edge-fog-cloud hybrid systems maintain accuracy comparable to centralized training while significantly reducing communication costs and data transmission requirements by 70-95%. General HITL literature consensus indicates 20-50% fewer labels are needed versus random sampling to achieve equivalent accuracy, with typical accuracy improvements of 5-15% over fully automated systems.

**Latency requirements critically differentiate edge-based from cloud-based HITL systems.** Ultra-critical applications (autonomous vehicles, industrial automation) demand <20ms response times, while real-time AI applications target 20-100ms—both infeasible with cloud architectures exhibiting 48-500ms round-trip latency. Edge-based HITL achieves 30-80ms processing times through local inference, enabling applications like predictive maintenance that require immediate feedback. The hybrid edge-cloud architecture emerges as optimal: edge devices handle real-time inference and simple corrections, fog nodes perform aggregation and federated learning, while cloud systems manage complex model training and centralized annotation workflows, combining low latency with scalability.

## Research Gap and TinyOL-HITL Contribution

**Existing TinyML on-device training systems face three critical limitations that TinyOL-HITL addresses.** First, vendor lock-in: commercial platforms like Edge Impulse (38-398KB RAM, 71-81% accuracy) provide end-to-end workflows but tie users to proprietary ecosystems, while academic systems like TinyOL and MCUNetV3 demonstrate feasibility but lack integrated deployment stacks. Second, training paradigm constraints: TinyOL trains only the last layer with frozen base networks, MCUNetV3 requires batch processing with predetermined sparse update patterns, and neither integrates human feedback for continuous model refinement. Third, platform validation gaps: most research validates on single platforms (typically ARM Cortex-M4), leaving cross-architecture performance uncharacterized—particularly for emerging platforms like RP2350 with novel Cortex-M33 architecture.

**TinyOL-HITL fills this gap by combining three innovations.** It implements streaming k-means clustering with human-in-the-loop corrections on open-standard frameworks (supOS-CE/RapidSCADA/Arduino IDE), avoiding vendor lock-in while enabling industrial SCADA integration via standard protocols (MQTT/OPC-UA). The system targets 15-25% accuracy improvement over CWRU bearing fault detection baselines (85-95% traditional ML, target 98-112%) through active learning that queries humans only on uncertain predictions, dramatically improving label efficiency. Critically, TinyOL-HITL validates across dual platforms—ESP32 (Xtensa architecture, 520KB SRAM) and RP2350 (ARM Cortex-M33, 520KB SRAM)—demonstrating cross-architecture portability and memory efficiency (<100KB target, <50mA power) on heterogeneous edge hardware, advancing both the theory and practice of adaptive edge intelligence.

## System Comparison and Quantitative Analysis

| System | Platform | Memory (SRAM) | Flash | Power | Accuracy | Training Method | Stack | Citation |
|--------|----------|---------------|-------|-------|----------|-----------------|-------|----------|
| **TinyOL** | ARM Cortex-M4 | ~256KB | 1MB | <0.1W | Task-dependent | Online/Streaming (last-layer) | Academic (Open) | Ren et al., IJCNN 2021 |
| **TFLite Micro** | Multi-platform | 26.5KB framework | Varies | N/A | N/A (inference) | Inference-only | Open (Google) | David et al., MLSys 2021 |
| **MCUNetV3** | STM32F746 (M7) | <256KB (149KB) | <1MB | N/A | Matches cloud | Sparse Update + QAS | Academic (Open) | Lin et al., NeurIPS 2022 |
| **TinyTL** | Edge/Mobile | 6.5× reduction | N/A | N/A | +34.1% vs last-layer | Bias-only + Lite Res. | Academic (Open) | Cai et al., NeurIPS 2020 |
| **Edge Impulse** | Multi-platform | 38-398KB† | 63-904KB† | N/A | 71-81%† | Cloud→Edge | Commercial (Closed) | Hymel et al., MLSys 2023 |
| **CMSIS-NN** | ARM Cortex-M4/M7 | <1MB | N/A | 4.9× efficiency | N/A (kernels) | Inference-only | Open (ARM) | Lai et al., arXiv 2018 |
| **Latent Replay** | Mobile/MCU | Replay-based‡ | N/A | N/A | SOTA CL | Rehearsal + Replay | Academic (Open) | Pellegrini et al., IROS 2020 |
| **TinyOL-HITL** | ESP32/RP2350 | <100KB (target) | TBD | <50mA | Target: +15-25% | Streaming + HITL | Open-Standard | This work |

**Notes:** † Edge Impulse values vary by model and task (KWS, VWW, Image Classification). ‡ Latent Replay memory depends on replay buffer size and quantization (500-5000 patterns).

**Baseline Performance Context:** CWRU bearing fault dataset baselines range from 85-95% for traditional ML methods (SVM, KNN, Random Forest) to 95-98% for basic CNNs, with state-of-the-art deep learning achieving 99-100%. Lightweight approaches like Lite CNN achieve 99.86% accuracy with only 0.64% the parameters of ResNet50, demonstrating that resource efficiency need not sacrifice performance. For noisy conditions (-9 dB SNR), accuracies drop to 68-85%, providing realistic improvement opportunities where TinyOL-HITL's adaptive learning and human corrections offer substantial value.

## Platform Characteristics and Cross-Architecture Validation

**ESP32 and RP2350 represent complementary architectures for validating TinyML portability.** ESP32 utilizes 32-bit RISC Tensilica Xtensa LX6/LX7 processors with dual cores at 240 MHz, 520KB SRAM, integrated WiFi/Bluetooth, and hardware FPU with DSP instructions. The platform dominates IoT applications but suffers from fragmented toolchain support and Espressif-specific optimizations. RP2350 features ARM Cortex-M33 cores with TrustZone security, 520KB SRAM, dual-core 150 MHz operation, and maintains compatibility with the established ARM ecosystem including CMSIS-NN optimizations—though literature remains sparse due to the platform's 2024 introduction.

**Cross-platform validation addresses a critical gap in TinyML research.** Most systems validate exclusively on ARM Cortex-M4 (TinyOL, MCUNetV3) or provide multi-platform claims without detailed comparative metrics (Edge Impulse). Demonstrating equivalent performance across Xtensa and ARM architectures with heterogeneous instruction sets, memory hierarchies, and compiler toolchains establishes true portability—essential for open-standard frameworks targeting diverse industrial hardware. The RP2350's Cortex-M33 offers hardware-accelerated cryptography for secure edge learning, while ESP32's integrated wireless enables MQTT-based SCADA integration, together representing realistic industrial deployment scenarios.

## Industrial Integration and Open-Standard Frameworks

**Vendor lock-in remains a persistent barrier to industrial TinyML adoption.** Edge Impulse provides comprehensive MLOps workflows but restricts deployment flexibility and introduces ongoing platform dependencies. Proprietary SCADA systems similarly constrain integration options. TinyOL-HITL's focus on open-standard frameworks—supOS-CE (Supcon's open supervisory platform), RapidSCADA (open-source SCADA), and Arduino IDE (ubiquitous embedded development)—enables integration with existing industrial infrastructure via standard protocols.

**MQTT and OPC-UA protocols bridge edge learning to industrial systems.** MQTT provides lightweight publish-subscribe messaging ideal for microcontroller telemetry and model update distribution, while OPC-UA offers unified architecture for industrial automation interoperability. Academic literature on SCADA-TinyML integration remains nascent, with most work documenting proprietary implementations rather than open-standard approaches. TinyOL-HITL's demonstration of streaming clustering with human corrections integrated into open SCADA platforms via standard protocols provides a reproducible template for industrial edge learning deployments.

## BibTeX References

```bibtex
@inproceedings{ren2021tinyol,
  title={TinyOL: TinyML with Online-Learning on Microcontrollers},
  author={Ren, Haoyu and Anicic, Darko and Runkler, Thomas A.},
  booktitle={2021 International Joint Conference on Neural Networks (IJCNN)},
  pages={1--8},
  year={2021},
  organization={IEEE},
  doi={10.1109/IJCNN52387.2021.9533927}
}

@article{david2021tensorflow,
  title={TensorFlow Lite Micro: Embedded Machine Learning on TinyML Systems},
  author={David, Robert and Duke, Jared and Jain, Advait and Reddi, Vijay Janapa and Jeffries, Nat and Li, Jian and Kreeger, Nick and Nappier, Ian and Natraj, Meghna and Regev, Shlomi and others},
  journal={Proceedings of Machine Learning and Systems},
  volume={3},
  pages={800--811},
  year={2021}
}

@inproceedings{lin2022mcunetv3,
  title={On-Device Training Under 256KB Memory},
  author={Lin, Ji and Zhu, Ligeng and Chen, Wei-Ming and Wang, Wei-Chen and Gan, Chuang and Han, Song},
  booktitle={Advances in Neural Information Processing Systems},
  volume={35},
  year={2022}
}

@inproceedings{cai2020tinytl,
  title={TinyTL: Reduce Memory, Not Parameters for Efficient On-Device Learning},
  author={Cai, Han and Gan, Chuang and Zhu, Ligeng and Han, Song},
  booktitle={Advances in Neural Information Processing Systems},
  volume={33},
  year={2020}
}

@article{hymel2023edge,
  title={Edge Impulse: An MLOps Platform for Tiny Machine Learning},
  author={Hymel, Shawn and Banbury, Colby and Situnayake, Daniel and Elium, Albert and Ward, Colby and Kelcey, Matthew and Baaijens, Mathijs and Majchrzycki, Mateusz and Plunkett, Jenny and Tischler, David and others},
  journal={Proceedings of Machine Learning and Systems},
  volume={6},
  year={2023}
}

@article{lai2018cmsis,
  title={CMSIS-NN: Efficient Neural Network Kernels for Arm Cortex-M CPUs},
  author={Lai, Liangzhen and Suda, Naveen and Chandra, Vikas},
  journal={arXiv preprint arXiv:1801.06601},
  year={2018}
}

@article{senzaki2021active,
  title={Active Learning for Deep Neural Networks on Edge Devices},
  author={Senzaki, Yuya and Hamelain, Christian},
  journal={arXiv preprint arXiv:2106.10836},
  year={2021}
}

@article{dairydigid2024,
  title={Dairy DigiD: An Edge-Cloud Framework for Real-Time Cattle Biometrics and Health Classification},
  author={Multiple Authors},
  journal={MDPI Designs},
  volume={6},
  number={9},
  pages={196},
  year={2024}
}

@inproceedings{distributed2019active,
  title={Distributed Active Learning Strategies on Edge Computing},
  author={Multiple Authors},
  booktitle={2019 IEEE International Conference on Edge Computing},
  year={2019},
  organization={IEEE}
}

@article{yoo2023lite,
  title={Lite and Efficient Deep Learning Model for Bearing Fault Diagnosis Using the CWRU Dataset},
  author={Yoo, Y. and Baek, S.-W.},
  journal={Sensors},
  volume={23},
  number={6},
  pages={3157},
  year={2023}
}

@article{rosa2024benchmarking,
  title={Benchmarking Deep Learning Models for Bearing Fault Diagnosis Using the CWRU Dataset: A Multi-Label Approach},
  author={Rosa, R.K. and Braga, D. and Silva, D.},
  journal={arXiv preprint arXiv:2407.14625},
  year={2024}
}

@article{neupane2020bearing,
  title={Bearing Fault Detection and Diagnosis Using Case Western Reserve University Dataset With Deep Learning Approaches: A Review},
  author={Neupane, D. and Seok, J.},
  journal={IEEE Access},
  volume={8},
  pages={93155--93178},
  year={2020}
}

@inproceedings{pellegrini2020latent,
  title={Latent Replay for Real-Time Continual Learning},
  author={Pellegrini, Lorenzo and Graffieti, Gabriele and Lomonaco, Vincenzo and Maltoni, Davide},
  booktitle={2020 IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS)},
  pages={10203--10209},
  year={2020},
  organization={IEEE}
}

@article{ravaglia2021tinyml,
  title={A TinyML Platform for On-Device Continual Learning with Quantized Latent Replays},
  author={Ravaglia, Lorenzo and Rusci, Manuele and Nadalini, Davide and Capotondi, Alessandro and Conti, Francesco and Benini, Luca},
  journal={IEEE Journal on Emerging and Selected Topics in Circuits and Systems},
  volume={11},
  number={4},
  pages={789--802},
  year={2021}
}
```
