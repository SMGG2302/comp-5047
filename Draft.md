# Project Proposal: HollowHaven

## Project Title

**HollowHaven: A Context-Aware Artificial Tree Hollow for Greater Gliders and Other Hollow-Using Species**

## Recommended Target Animal

Our project will be designed primarily for the **Southern/Central Greater Glider** (*Petauroides volans*), an endangered Australian arboreal marsupial found in eucalypt forests across eastern Australia. This is a strong species choice for a shelter-based device because the need is both urgent and technically appropriate: greater gliders depend on large, old tree hollows for daytime shelter, breeding, and protection, but these hollows are being lost through land clearing, logging, severe bushfires, and climate-related heat stress.

However, the project will not treat the hollow as a single-species container. In real Australian forests, artificial hollows may also be visited by microbats, small possums, parrots, insects, or invasive species. HollowHaven will therefore use the greater glider as the **primary design stakeholder**, while recognising other hollow-using animals as **secondary stakeholders** whose presence should be detected, logged, and responded to carefully. This gives the project a focused design target without ignoring the multi-species reality of tree-hollow habitats.

Compared with a general wildlife shelter, a greater-glider-focused shelter gives the project a clearer ecological argument. The species is threatened, hollow-dependent, nocturnal, sensitive to temperature, and difficult for humans to monitor without disturbance. At the same time, detecting other visitors allows the system to explore habitat competition, non-target occupancy, and ethical multi-species management. This makes it well suited to a pervasive computing system that can sense environmental conditions, infer animal presence, and adapt the shelter without requiring a screen-based interface.

## Challenge Statement

Australia's hollow-bearing trees are slow-forming ecological infrastructure. Many native animals depend on them, but mature trees with suitable hollows can be reduced by clearing, timber harvesting, storms, and high-severity fire. Greater gliders are especially vulnerable because they require tree hollows as den sites and are affected by overheating and habitat fragmentation. Traditional nest boxes can help, but they may become too hot, too cold, too wet, or be occupied by non-target species. They also often require human inspection, which can disturb animals and provides only occasional data.

The design challenge is therefore:

**How might we design a more-than-human shelter that prioritises greater gliders' need for safe, thermally stable hollow habitat while also recognising and respectfully responding to other animals that may use or approach the same hollow?**

## Non-Human Stakeholder Analysis

The primary non-human stakeholder is the greater glider. Its needs include a dark and enclosed daytime refuge, stable internal temperature, low disturbance, safe entry, predator avoidance, and conditions suitable for resting or breeding. The device should treat the greater glider not as a passive object being monitored, but as a participant whose presence, movement, and comfort shape the system's behaviour.

Secondary non-human stakeholders include eucalypt forest trees, insects, microclimate, and other hollow-using species such as small possums, microbats, and birds. These actors matter because artificial hollows can alter local habitat competition. The system should therefore recognise signs of non-target occupancy and respond in low-risk ways, such as changing the external maintenance signal, adjusting ventilation only when safe, or logging the event for later human review. It should not aggressively exclude native animals, because other species may also have legitimate shelter needs.

Potential conflict stakeholders include invasive or risky visitors, such as feral cats near the entrance, aggressive introduced birds, or insects blocking drainage or ventilation. For these cases, the system's response should still be humane and cautious: it may alert human carers or temporarily protect the entrance in the prototype, but should not harm animals. Human stakeholders include conservation volunteers, park staff, wildlife carers, and researchers who may install, maintain, and interpret the shelter.

The system will infer non-human needs through environmental and behavioural cues rather than direct human observation:

- Temperature and humidity indicate whether the hollow is becoming thermally stressful or damp.
- PIR or time-of-flight motion sensing indicates entry, exit, or internal activity.
- Low-light or infrared camera/audio classification can help distinguish likely greater-glider use from empty shelter, native non-target visitors, or possible invasive/predator activity.
- External light and time-of-day data help the device avoid disruptive actuation during daytime resting periods.

## Project Concept

**HollowHaven** is a smart artificial hollow designed as a small-scale prototype of a conservation shelter primarily tuned for greater gliders, while still acknowledging that tree hollows are shared ecological spaces. It resembles a section of eucalypt trunk rather than a generic box. The outer shell is rough, bark-like, weather-resistant, and vertically mounted. The inner chamber is dark, insulated, and shaped to mimic the enclosed feeling of a natural tree hollow.

The system continuously senses internal temperature, humidity, light level, and movement. When the internal microclimate becomes risky, it responds through quiet, non-screen actuation:

- A small servo opens or closes hidden ventilation fins to regulate heat and humidity.
- A low-noise fan can briefly increase airflow when the shelter is unoccupied or when heat risk is high.
- A soft external LED or e-ink status mark communicates maintenance needs to humans without requiring them to open the shelter.
- Optional vibration/ultrasonic deterrence can be explored only for non-harmful predator or invasive-species boundary signalling, and only in a demonstration-safe form.

The AI component will support context recognition. For the prototype, an AI vision or audio API can classify short captured events into categories such as "likely greater glider", "native non-target visitor", "possible predator/invasive visitor", or "empty/environmental noise". In the classroom demo, this can be tested with printed animal images, short videos, or controlled movement/audio examples instead of live wildlife. The AI output will not make high-risk decisions alone; it will combine with sensor thresholds to select a safe system state.

The interaction design is intentionally ambient. The greater glider interacts by occupying, entering, leaving, and changing the hollow's sensed conditions. Other animals also become part of the interaction when their presence changes the system state. Humans interact through installation, maintenance, and interpretation of external signals. The device avoids becoming a screen dashboard and instead behaves like a quiet field instrument embedded in Country.

## Components

Planned hardware:

- **Microcontroller:** ESP32, chosen for Wi-Fi capability, low cost, and compatibility with Arduino-style development.
- **Sensors:** DHT22 or SHT31 temperature/humidity sensor; PIR motion sensor or VL53L0X time-of-flight sensor; light sensor such as BH1750; optional microphone module or ESP32-CAM for AI-triggered classification.
- **Actuators/feedback:** Micro servo for ventilation fins; low-noise 5V fan; RGB LED or small LED matrix for ambient maintenance status; optional buzzer used only for human setup/testing, not animal-facing use.
- **Power:** USB power bank for prototype; future field version could use solar charging and deep-sleep firmware.
- **Enclosure:** CAD-designed artificial hollow using 3D printed shell sections, timber/bark-textured cladding, removable maintenance panel, insulated inner chamber, drainage, and shaded entry tunnel.
- **AI/API:** Image or audio classification API to interpret event snapshots or short recordings as likely target species, native non-target visitor, possible predator/invasive visitor, or environmental noise; optional weather API to anticipate heatwave conditions and shift the shelter into heat-protection mode earlier.

## System States

- **Resting Mode:** Low light, no recent movement, stable temperature. Device logs data quietly.
- **Likely Greater Glider Mode:** Internal movement or AI classification suggests target-species use during expected nocturnal/denning patterns. Actuation becomes slower and quieter to avoid disturbance, and microclimate control prioritises stable, dark refuge conditions.
- **Native Visitor Mode:** AI or sensor pattern suggests another native hollow-using animal. The system logs the visit, avoids harmful exclusion, and uses only low-risk ventilation or external maintenance signalling.
- **Risk Visitor Mode:** Sensor or AI pattern suggests a possible predator, invasive visitor, or entrance blockage. The system alerts human carers through the external signal and may enter a protective demonstration state, such as temporarily narrowing an entrance flap in the prototype.
- **Heat-Risk Mode:** Internal temperature exceeds threshold or external forecast indicates heatwave. Vent fins open, fan activates only if movement suggests the hollow is empty or if heat becomes critical.
- **Damp-Risk Mode:** Humidity remains high. Ventilation increases and external maintenance signal changes.
- **Maintenance Mode:** External LED pattern indicates sensor fault, water ingress risk, battery low, or unusual occupancy pattern.

## More-Than-Human Framing

The project uses **Animal-Computer Interaction** and **more-than-human design**. The key design question is not only whether humans can collect data, but whether the shelter can respond respectfully to a non-human resident's rhythms. The greater glider's nocturnal behaviour, need for darkness, vulnerability to heat, and dependence on old-tree hollows become the primary design requirements. The system also recognises that other species may appear, so the design becomes a form of multi-species negotiation rather than a rigid single-animal device.

The project can also acknowledge **Country-centred design** by framing the shelter as part of a wider living relationship between eucalypt forests, hollow-bearing trees, fire regimes, animals, and human caretakers. If possible, the final report should identify the specific Country where the imagined deployment is located and avoid presenting technology as a replacement for habitat protection.

## Risks and Open Questions

- **Ethical risk:** The prototype must not be tested with wild animals without approval. Classroom testing should use simulations, controlled objects, or publicly available media.
- **Ecological risk:** Artificial shelters are not a substitute for protecting mature hollow-bearing trees. The proposal should present the device as supplementary support and monitoring.
- **Multi-species risk:** Detecting other species does not automatically mean they should be excluded. The project must distinguish between logging, gentle adaptation, human notification, and active deterrence.
- **Thermal risk:** Poorly designed nest boxes can overheat. The enclosure must prioritise insulation, shade, ventilation, and internal temperature testing.
- **AI reliability:** AI classification may misidentify species. It should support logging, broad visitor categories, and low-risk adaptation, not make dangerous autonomous decisions.
- **Fabrication risk:** A realistic hollow shape may be difficult to print in one piece. The design should use modular sections with an accessible electronics bay.
- **Power risk:** Fans and cameras consume energy. The prototype can use USB power, while the report discusses low-power field deployment.

## Preliminary Timeline

| Period | Work |
| --- | --- |
| Week 4 | Submit proposal; confirm target species, sensors, and enclosure concept. |
| Weeks 5-6 | Build ESP32 sensing prototype; test temperature, humidity, light, and motion logging. |
| Weeks 7-8 | Add servo ventilation and ambient LED feedback; prototype AI classification using sample images/audio. |
| Week 9 | Mid-project check-in; demonstrate context-recognition states and early enclosure model. |
| Weeks 10-11 | Fabricate enclosure; integrate sensors, wiring, ventilation fins, and inner insulation. |
| Weeks 11-12 | Test heat/damp scenarios, tune thresholds, write report and user manual, film demo. |
| Week 13 | Final live demonstration and more-than-human design reflection. |

## Why This Direction Is HD-Worthy

This concept is stronger than a simple animal box because it integrates ecological evidence, non-human-centred design, context recognition, AI-supported sensing, physical actuation, and custom fabrication. It also gives the presentation a clear story: Australia has a shortage of safe tree hollows; greater gliders are endangered and sensitive to habitat and climate pressures; HollowHaven explores how a shelter can prioritise one threatened species while still recognising the wider community of animals that share hollow habitats.

## Initial References

- WWF-Australia: Greater gliders were listed as Endangered in 2022 and some populations have declined by up to 80% due to habitat destruction, logging, deforestation and climate-fuelled bushfires. https://wwf.org.au/what-we-do/species/greater-glider/
- Threatened Species Recovery Hub: More than 200 Australian vertebrate species depend on natural tree hollows and branch cavities for nesting, denning, and predator refuge. https://www.nespthreatenedspecies.edu.au/projects/testing-the-effectiveness-of-nest-boxes-for-threatened-species
- ACT Government: Southern Greater Glider threats include habitat destruction, severe wildfire, changed fire regimes, heatwaves, climate change, land clearing and timber harvesting. https://www.act.gov.au/environment/animals-and-plants/act-threatened-species/southern-greater-glider-petauroides-volans
- Greening Australia: Existing hi-tech greater glider nest box projects use insulation, air gaps, and heat-reflective, fire-resistant, non-toxic coatings to improve post-bushfire shelter. https://www.greeningaustralia.org.au/greater-glider-families-move-into-hi-tech-nest-boxes/
