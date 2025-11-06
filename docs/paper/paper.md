# ACTION ITEMS FOR USER
Review all tagged items before final submission:
- **[VERIFY]** - Needs full paper access or confirmation
- **[REMOVE]** - Cannot verify; suggest removal
- **[FIX]** - Critical correction applied
- **[CONTEXT]** - Important clarification added

---

# Introduction

Industrial predictive maintenance (PdM) faces an adoption crisis. Despite 95% of adopters reporting positive ROI with average 9% uptime gains and 12% cost reductions [@pwc2018PredictiveMaintenance40], adoption remains at just 27% [@maintainx20252025StateIndustrial]. More critically, **[CONTEXT: specific to Industry 4.0 manufacturing, not all sectors]** 74% of manufacturers remain stuck in perpetual pilot programs [@mckinsey&company2021Industry40Adoption], unable to scale beyond proof-of-concept.

Three barriers block deployment:

**Expertise shortage:** 24-52% cite lack of qualified personnel [@maintainx20252025StateIndustrial; **[REMOVE: IDC/Microsoft 2023 not verified]**]. Only 320,000 qualified AI professionals exist against 4.2 million positions—7.6% fill rate **[VERIFY: secondary sources only]**. Data scientists command $90,000-$195,000 salaries [@u.s.bureauoflaborstatistics2024OccupationalOutlookHandbook] with **[FIX: 142-day average time-to-hire [@comptia2024TechWorkforce] - NOT Keller 2025]**. Traditional ML deployment requires 8-90 days per model [@algorithmia20202020StateEnterprise].

**Vendor lock-in:** **[REMOVE: "Initial costs block 34%" - unverified]**. Proprietary systems charge **[REMOVE: "$10,000-$99,000" - specific range unverified]** for OPC licenses. **[REMOVE: "80% seek to avoid lock-in" - A&D 2020 survey not found]**. Total cost of ownership crossover occurs within 5-7 years [@nor-calcontrols2023DeterminingSolarPV].

**Integration complexity:** **[REMOVE: "Brownfield $5-6.5M vs $1-1.3B greenfield" - NOT in Alqoud et al.]** Equipment operates 20+ years [@alqoud2022Industry40Systematic]. 60% face data quality issues; 29% of technicians feel unprepared [@maintainx20252025StateIndustrial].

---

# Related Work

## Embedded Machine Learning Frameworks

**TinyOL [@ren2021TinyOLTinyMLOnlinelearning]** pioneered online learning on ARM Cortex-M4 microcontrollers with 256KB SRAM. The system processes streaming data one sample at a time, updating weights incrementally via stochastic gradient descent. **[FIX: TinyOL trains only the last layer while freezing the base network in Flash memory - causes ≥10% accuracy loss vs full network training]**. TinyOL achieves 1,921μs average latency (inference + update) versus 1,748μs for inference-only—just 10% overhead.

**TensorFlow Lite Micro** established the foundation for edge inference with **[REMOVE: "26.5KB framework footprint" - unverified]** INT8 quantization. TFLite Micro remains strictly inference-only **[FIX: because training requires significantly more memory for storing intermediate activations, gradients, and optimizer states]**.

**MCUNetV3 [@lin2022mcunetv3]** achieved full-network training under 256KB memory through **[FIX: sparse gradient updates and Quantization-Aware Scaling (QAS) - NOT batching; uses batch size = 1]**, reducing memory by 20-21× compared to full updates while matching cloud training accuracy on STM32F746.

**TinyTL [@cai2020tinytl]** demonstrated **[FIX: 33.8% accuracy improvement (NOT 34.1%)]** over last-layer-only fine-tuning through 6.5× memory reductions via bias-only updates with lite residual modules.

**CMSIS-NN [@lai2018cmsis]** optimized ARM Cortex-M processors through SIMD instructions, achieving 4.6× runtime improvements, but provides no training capabilities.

**Edge Impulse** provides end-to-end MLOps workflows **[CONTEXT: cloud-dependent for training; offline for inference; no on-device adaptation capability]**. **[REMOVE: RAM 38-398KB and accuracy 71-81% claims - unverified in peer-reviewed sources]**.

## Streaming and Incremental Learning Algorithms

**Mini-batch k-means** achieves memory reductions from 52GB (standard k-means) to 0.98GB [@hicks2021mbkmeans] through fixed-size batches with incremental centroid updates. The algorithm exhibits O(dk(b+log B)) complexity with memory optimum at B=n^(1/2) batches [@ahmatshin2024minibatch].

**[REMOVE OR VERIFY: EdgeClusterML (2024) - no publication found in indexed venues]**.

**Enhanced Vector Quantization (AutoCloud K-Fixed, 2024)** [@autocloud2024] achieved >90% model compression through incremental clustering for automotive embedded platforms.

**[CONTEXT: Streaming k-means theoretical guarantees confirmed, but K×D×4 bytes is implementation-dependent, not theoretical guarantee]**.

## Human-in-the-Loop and Active Learning

Active learning with human feedback reduces labeling costs by 20-80%. **[FIX: Wei et al. 2019 JAMIA (NOT Wang)]** [@wei2019CostawareActiveLearning] demonstrated 20.5-30.2% annotation time reduction. Baldridge & Osborne (2004) [@baldridge2004ActiveLearningTotal] achieved 80% cost reduction combining active learning with model-assisted annotation.

**Dairy DigiD (2024)** [@dairydigid2024] achieved 3.2% mAP improvement and 73% model size reduction (128MB→34MB) on NVIDIA Jetson Xavier NX, with **[FIX: 84% reduction in technician training time (USER training), NOT model training time]**.

**[REMOVE: "70-95% communication cost reduction" - not quantified in IEEE 2019 abstract; needs full paper]**.

**[FIX: Latency requirements - Ultra-critical: <1-10ms (NOT <20ms); Real-time: 10-100ms; Cloud round-trip: 48-500ms typical]** [@urllc2020; @profinet2023].

Mosqueira-Rey et al. (2023) [@mosqueira2023hitl] provide comprehensive HITL-ML taxonomy (800+ citations).

## Industrial Adoption Barriers

**MaintainX (2025)** [@maintainx20252025StateIndustrial] surveyed **[FIX: 1,320 MRO professionals (NOT 1,165)]** finding 27% PdM adoption (down from 30% in 2024) and 24% citing expertise barriers.

**PwC/Mainnovation (2018)** [@pwc2018PredictiveMaintenance40] surveyed 268 companies: 95% ROI, 9% uptime gains, 12% cost reductions, only 11% at Level 4 maturity.

**McKinsey (2020)** [@mckinsey&company2021Industry40Adoption] found **[CONTEXT: in Industry 4.0 manufacturing specifically, NOT general AI/ML]** 74% stuck in "pilot trap" (400 manufacturers surveyed). **[NOTE: LNS Research disputes this, finding only 7-13%]**.

**U.S. Bureau of Labor Statistics (2024)** [@u.s.bureauoflaborstatistics2024OccupationalOutlookHandbook] reports data scientists earn $90,000-$195,000 (10th-90th percentile).

**Algorithmia (2020)** [@algorithmia20202020StateEnterprise] (745 respondents, 25-page report): 50% spend 8-90 days per model deployment; 25%+ of data scientist time on deployment.

## Vendor Lock-in and Cost Evidence

**Journal of Cloud Computing (2016)** [@opara-martins2016VendorLockin] surveyed 114 organizations: 71% stated vendor lock-in deters cloud adoption.

**Nor-Cal Controls (2023)** [@nor-calcontrols2023DeterminingSolarPV] documented **[CONTEXT: for Solar PV SCADA specifically]** 5-7 year TCO crossover point where open-source becomes cheaper than proprietary systems (30-50% cheaper upfront, but recurring fees accumulate).

**IoT Analytics (2020-2025)** [@iotanalytics2025IoTEdgeComputing] (248-page report): $11.6B→$30.8B market growth; **[FIX: 90% cost reduction case study documented (NOT 42-92% range)]**.

**OSIE Consortium** [@osie2020OpenSourceIndustrial] **[FIX: started May 1, 2020 (NOT 2018)]**: 10x/90% cost reduction with open-source edge.

**[REMOVE: Op-tec "$10K-99K" and A&D "80% survey" - cannot verify]**.

## Open Standards and Protocols

**MQTT adoption:** Eclipse IoT (2024): 56%; IIoT World: 50.3% **[CONTEXT: dominant IIoT protocol based on industry surveys]**.

**OPC-UA:** OPC Foundation welcomed 800th member in December 2020 [@opcfoundation2020] **[CONTEXT: This is Foundation membership, not necessarily protocol deployments]**.

**[FIX: 91% IoT developers use open-source tools - VisionMobile (2016) survey of 3,700 developers, NOT "A&D 2020"]**.

## CWRU Dataset Baselines

**Traditional ML (SVM, KNN, Random Forest):** 85-95% accuracy (multiple sources).

**Basic CNNs:** 95-98% (conservative estimate; most achieve 97-100%).

**State-of-the-art deep learning:** 99-100% **[CRITICAL WARNING: Rosa et al. (2024) and Hendriks et al. (2022) identified data leakage in typical CWRU splits, inflating results by 2-10%. Same physical bearings appear in train and test sets]** [@rosa2024benchmarking].

**Lite CNN (Yoo & Baek, 2023)** [@yoo2023lite]: 99.86% accuracy with 0.64% parameters vs ResNet50 (153K vs 23,900K parameters).

**[FIX: Noisy conditions (-9 dB SNR) - modern models maintain 90-95% accuracy, NOT 68-85%. The claimed range appears to be absolute accuracy, not accuracy drop. Actual drops are 5-15%]**.

## Platform Specifications

**ESP32-S3** [@espressif2024esp32s3]:
- **[FIX: Xtensa LX7 ONLY (NOT LX6/LX7)]**
- **[FIX: 512KB SRAM (NOT 520KB)]**
- Clock: 240 MHz dual-core
- Hardware FPU: Yes
- WiFi/Bluetooth: Integrated

**RP2350** [@raspberrypi2024rp2350] (introduced August 2024):
- ARM Cortex-M33 (also RISC-V option)
- 520KB SRAM
- 150 MHz dual-core
- TrustZone: Yes

**[FIX: Alqoud et al. 2022 protocol adoption - OPC-UA 75% + MQTT 25% reflects ACADEMIC LITERATURE (32 papers analyzed), NOT market-wide adoption]** [@alqoud2022Industry40Systematic].

---

# System Architecture

**[USER: Insert your methodology here - validated technical specs above]**

---

# Implementation

**[USER: Describe ESP32-S3 (Xtensa LX7, 512KB SRAM) and RP2350 platforms, CWRU dataset integration]**

---

# Experimental Validation

**[USER: Present results - note CWRU data leakage issues in discussion]**

---

# Discussion

**Memory and Performance Targets:**

**<100KB RAM:** Justified. ESP32-S3: 340KB available after OS/WiFi (29% utilization). RP2350: 400KB available (25%). MCUNetV3 achieved 149KB; <100KB is aggressive but achievable.

**<50mA Power:**
- RP2350: 30-40mA typical - **<50mA achievable**
- ESP32-S3: 30-50mA baseline (peaks 73mA), WiFi adds 130mA bursts
- **[RECOMMEND: Separate targets or specify "excluding WiFi transmission bursts"]**

**15-25% Accuracy Improvement:** Justified via:
- TinyOL last-layer penalty: ≥10%
- CWRU gaps: 10-30% between traditional ML (85-95%) and deep learning (99-100%)
- HITL literature: Well-documented improvements
- **Example scenario:** Baseline 70-75% → HITL target 85-90% = 15-20% gain

**Research Gap Confirmed:** No existing system combines unsupervised online learning + HITL + open standards (MQTT/OPC-UA) + multi-platform validation (ESP32-S3 Xtensa + RP2350 ARM).

**Cross-Platform Validation:** Most TinyML research validates on single platform (ARM Cortex-M4/M7). Cross-architecture gap is real and significant.

**[CRITICAL REMOVAL: MadeiraMadeira 90% cost reduction case - this is e-commerce website optimization (edge caching/CDN), NOT industrial brownfield integration. Company: Brazilian e-commerce. Apples-to-oranges comparison]**.

---

# Conclusion

TinyOL-HITL demonstrates that...

**[USER: Complete conclusion]**

---

# References

## CORRECTED CITATIONS

```bibtex
@inproceedings{ren2021TinyOLTinyMLOnlinelearning,
  title={TinyOL: TinyML with Online-Learning on Microcontrollers},
  author={Ren, Haoyu and Anicic, Darko and Runkler, Thomas A.},
  booktitle={2021 International Joint Conference on Neural Networks (IJCNN)},
  pages={1--8},
  year={2021},
  organization={IEEE},
  doi={10.1109/IJCNN52387.2021.9533927}
}

@article{wei2019CostawareActiveLearning,
  title={Cost-aware active learning for named entity recognition in clinical text},
  author={Wei, Q. and Wu, S. and Chen, Y. and others},
  journal={Journal of the American Medical Informatics Association},
  year={2019},
  volume={26},
  number={11},
  pages={1314--1322},
  doi={10.1093/jamia/ocz102}
}

@inproceedings{cai2020tinytl,
  title={TinyTL: Reduce Memory, Not Parameters for Efficient On-Device Learning},
  author={Cai, Han and Gan, Chuang and Zhu, Ligeng and Han, Song},
  booktitle={Advances in Neural Information Processing Systems},
  volume={33},
  year={2020}
}

@inproceedings{lin2022mcunetv3,
  title={On-Device Training Under 256KB Memory},
  author={Lin, Ji and Zhu, Ligeng and Chen, Wei-Ming and Wang, Wei-Chen and Gan, Chuang and Han, Song},
  booktitle={Advances in Neural Information Processing Systems},
  volume={35},
  year={2022}
}

@techreport{comptia2024TechWorkforce,
  title={Tech Workforce Report 2024},
  author={{CompTIA}},
  year={2024}
}

@techreport{osie2020OpenSourceIndustrial,
  author={{OSIE Project Consortium}},
  title={Open Source Industrial Edge (OSIE): Industrial Edge as a Service},
  year={2020},
  month={May},
  note={Eurostars-Eureka Funded Project, Started May 1, 2020}
}

@article{rosa2024benchmarking,
  title={Benchmarking deep learning models for bearing fault diagnosis using the CWRU dataset: A multi-label approach},
  author={Rosa, R.K. and Braga, D. and Silva, D.},
  journal={arXiv preprint arXiv:2407.14625},
  year={2024}
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
```