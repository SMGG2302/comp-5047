# Project Proposal: HollowHaven

## Project Title

**HollowHaven: A Context-Aware Artificial Tree Hollow for Thermally Supporting Southern/Central Greater Gliders**

## Challenge Statement

The Southern/Central Greater Glider (*Petauroides volans*) is an endangered, nocturnal marsupial that depends on large hollows in mature eucalypts for daytime refuge and breeding. These slow-forming hollows are being reduced by clearing, logging and severe bushfire. Artificial hollows can provide supplementary habitat, but their internal temperatures can be more variable and extreme than those of natural hollows.

Gracanin et al. (2025) found that, on warm days, the best artificial nest-box design averaged 1.99 degrees C cooler than ambient temperature, compared with 7.32 degrees C for natural hollows used by greater gliders. Brown et al. (2026) also found less fluctuation in natural hollows; at an ambient maximum of 39.5 degrees C, predicted temperatures were 34.3 degrees C in nest boxes and 31.7 degrees C in hollows. The Australian Government's 2026 National Recovery Plan identifies artificial-hollow depth, orientation, thermal buffering, structural features, testing and adaptive monitoring as research priorities.

HollowHaven focuses on one measurable need: reducing thermal extremes and approximating natural-hollow buffering under changing conditions.

**Design challenge:** How might a context-aware artificial hollow maintain a more stable thermal microclimate for Southern/Central Greater Gliders by sensing environmental change and adaptively controlling ventilation?

**Research question:** Can HollowHaven reduce internal temperature extremes and more closely approximate natural-hollow thermal buffering than a conventional passive artificial shelter?

**Hypothesis:** Under the same external heat exposure, HollowHaven will produce a lower maximum internal temperature, a smaller temperature range and a longer thermal lag than a conventional artificial hollow.

## Non-Human Stakeholder Analysis

The primary stakeholder is the greater glider, which needs a dark, deep refuge with a stable daytime microclimate, low disturbance and safe entry. It is treated as a participant rather than only a monitored object: inferred occupancy changes how cautiously the system actuates.

Other hollow users, insects, trees and the microclimate are secondary stakeholders. The system infers only occupied or unoccupied conditions and never automatically excludes non-target native animals. Conservation volunteers or researchers receive an external maintenance signal and can service the electronics without opening the refuge chamber.

Context combines internal and shaded external temperature and humidity, light as a solar-exposure proxy, time, temperature trend and occupancy. The response depends on whether the chamber is heating or cooling, whether outside air can provide cooling and whether an animal may be present.

## Project Concept

HollowHaven is a vertically mounted scale model with an insulated deep cavity, shaded entrance, drainage and separate maintenance bay. Its depth-to-width relationship is informed by greater-glider den morphology rather than a shallow bird-box form. Modular CAD sections allow servicing without disturbing the refuge chamber.

Two SHT31 sensors measure internal and shaded external microclimates. A BH1750 indicates radiant exposure, while a VL53L0X detects entry and occupancy. The ESP32 combines:

`Context = (T_inside, T_outside, RH_inside, RH_outside, light, time, temperature slope, occupancy)`

The primary actuator is a servo-controlled passive vent that opens gradually when temperature is rising and outside air can provide cooling. A low-noise fan is reserved for heat-protection tests because it adds power, noise and failure costs. An external RGB LED communicates normal, heat-protection or maintenance status.

A lightweight predictive model accessed through an inference API will estimate internal temperature 20-30 minutes ahead from recent readings. It can open the vent before the predicted peak, while local fallback logic operates if the API fails.

## Context-Aware System States

| State | Context | Response |
| --- | --- | --- |
| **Normal Refuge** | Stable temperature and no developing heat load | Vent mostly closed; low-frequency sensing and logging |
| **Warming** | Light and temperature slope indicate increasing heat load | Vent opens gradually when external conditions make ventilation useful |
| **Heat Protection** | Predicted or measured temperature is high or rising rapidly | Vent opens further; logging frequency increases; fan is used only under the prototype's safety rules |
| **Recovery** | External and internal temperatures are falling | Vent closes gradually with hysteresis to avoid rapid cycling or excessive cooling |

Occupancy modifies every state by making actuation slower and quieter. Control thresholds are experimental settings tuned through comparison, not claims of biological injury limits.

## Components

- **Microcontroller and logging:** ESP32 and microSD module.
- **Sensors:** two SHT31 temperature/humidity sensors (internal and shaded external reference), BH1750 light sensor and VL53L0X time-of-flight sensor.
- **Actuation and feedback:** micro servo for ventilation fins, low-noise 5 V fan as a secondary actuator and RGB LED for ambient human-readable status.
- **AI/API:** lightweight thermal forecasting model exposed through an inference API; local fallback logic if connectivity fails.
- **Power:** USB power bank for the classroom prototype; a field version would require low-power firmware and solar feasibility testing.
- **Enclosure:** team-designed CAD model with a deep vertical chamber, insulated wall system, shaded entrance, drainage, hidden ventilation path and separate electronics access.

## Evaluation Plan

Three equal-scale chambers will receive repeated, controlled radiant heat exposure: (A) basic plywood, (B) insulated passive and (C) adaptive HollowHaven. They share the same heat conditions and external reference; no live animals are used.

The comparison will measure:

- maximum internal temperature, `T_max`;
- experimental temperature range, `T_max - T_min`;
- thermal buffering while ambient air is hotter, `B = T_ambient - T_inside`;
- time above declared evaluation thresholds, such as `t(T > 30 degrees C)` and `t(T > 35 degrees C)`; and
- thermal lag between the external and internal temperature peaks.

These thresholds are comparison markers, not biological harm claims. Success means repeatedly reducing peak temperature and fluctuation relative to the passive control and moving the curve toward literature-derived natural-hollow behaviour. Prediction error, response time and API fallback will also be recorded.

## More-Than-Human Design Rationale

HollowHaven applies Animal-Computer Interaction and more-than-human design by translating the need for low-disturbance thermal refuge into its form and control rules. Occupancy slows actuation, passive control is preferred, ambiguous data produces conservative responses and native visitors are not automatically excluded. It supplements rather than replaces mature hollow-bearing trees.

## Risks and Open Questions

- **Ethics:** Only physical simulations will be used; wildlife deployment requires expert approval.
- **Validity:** Scale-model comparison cannot directly prove field safety or animal benefit.
- **Safety:** Ventilation must avoid rain, overcooling, trapping and rapid movement.
- **AI reliability:** Local sensing and fail-safe logic continue if prediction is inaccurate or unavailable.
- **Materials:** Geometry, orientation, ventilation and the wall system must be tested together; insulation is not assumed to solve overheating alone.
- **Fabrication and power:** Service access and fan energy use require validation.

## Preliminary Timeline

| Period | Work |
| --- | --- |
| Week 4 | Submit proposal; confirm components, research question and test protocol. |
| Weeks 5-6 | Build dual-SHT31, light and occupancy sensing; implement logging and baseline tests. |
| Weeks 7-8 | Build passive and adaptive control logic; integrate servo, LED and inference API; begin CAD. |
| Week 9 | Demonstrate the four states, preliminary temperature curves and response to tutor feedback. |
| Weeks 10-11 | Fabricate and integrate the three test chambers; run repeated heat-exposure experiments. |
| Weeks 11-12 | Analyse metrics, tune control logic, document failures and iterations, and prepare report, manual and video. |
| Week 13 | Present the live comparative experiment and more-than-human reflection. |

## Initial References

### Ecological Motivation

- DCCEEW (2026), *National Recovery Plan for Greater Gliders*. https://www.dcceew.gov.au/environment/biodiversity/threatened/publications/recovery/greater-gliders
- Gracanin, A. et al. (2025), *Rapid Uptake of Nest Boxes by the Endangered Greater Glider (Petauroides volans)*. https://doi.org/10.1111/emr.70000
- Brown, T. J. et al. (2026), *Supplementary habitat use by endangered southern greater gliders (Petauroides volans)*. https://doi.org/10.1071/PC25043

### Design Rationale and Similar Work

- Howard, I. et al. (2022), *Helping wildlife beat the heat: Testing strategies to improve the thermal performance of nest boxes*. https://doi.org/10.7882/AZ.2022.026
- Hofman, M., Gracanin, A. and Mikac, K. M. (2022), *Greater glider (Petauroides volans) den tree and hollow characteristics*. https://doi.org/10.1071/AM22008
