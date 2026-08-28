# Project Proposal: HollowHaven

## Project Title

**HollowHaven: A Context-Aware Artificial Tree Hollow for Greater Gliders and Other Hollow-Using Species**

## Challenge Statement

HollowHaven is designed primarily for the endangered Southern/Central Greater Glider (*Petauroides volans*), which depends on mature eucalypt hollows for shelter and breeding. Clearing, logging, severe bushfire and climate-related heat are reducing suitable hollows. Conventional nest boxes can help, but many are shallow, overheat, retain moisture, attract non-target species or require disruptive inspection.

The design challenge is:

**How might we create a low-disturbance artificial hollow that buffers environmental extremes, responds to animal and microclimate conditions, prioritises greater gliders and still treats other hollow-using species ethically?**

## Evidence-Led Design

The strongest quantitative natural-den dataset comes from Seven Mile Beach, NSW. Across 68 used hollows in 54 trees, mean DBH was 114 cm, hollow height 15.4 m, depth 2.52 m, minimum entrance dimension 18.1 cm and maximum wall thickness 8 cm. Deep vertical branch-end hollows were most common.

A Commonwealth synthesis reports depths of 0.3–5.0 m (mean 2.5 m) and wall thicknesses of 1–30 cm (mean 8 cm). A 2026 study of 16 dens recorded 9–30 cm entrances, 0.7–4.0 m depths and 1–21 cm walls. Natural dens are therefore deeper and more irregular than conventional boxes.

No species-specific optimum for internal volume is known. An occupied 2026 artificial box measured about 23 × 25 × 35–43 cm (0.022 m³). This proves that a smaller cavity can be used, not that it is optimal or thermally equivalent to a living-tree hollow.

Greater-glider heat dissipation becomes constrained above about 20°C, but this is not a daytime-den limit. Occupied natural dens have averaged 24.7°C daily maxima, and occupied artificial and natural hollows have reached 34.0°C and 34.7°C. Occupancy indicates use, not optimality.

Living-tree hollows can remain 7–8°C cooler than ambient and fluctuate less. In 2025, a well-performing timber box averaged 1.99°C below ambient versus 7.32°C for occupied natural hollows. A 2026 model predicted 34.3°C in a box and 31.7°C in a natural hollow at 39.5°C ambient.

## Proposed Solution

HollowHaven is a vertically mounted, bark-textured artificial hollow with a dark, insulated and drained internal chamber. The first-generation standard prototype will use:

| Feature | Proposed specification |
| --- | --- |
| Internal cross-section | Approximately 230–280 × 250–300 mm |
| Effective chamber depth | At least 400–450 mm |
| Entrance | 120–130 mm circular opening or equivalent short axis |
| Form | Vertical chamber; rear/lower entrance; no exposed perch |
| Future installation height | Target 10–15 m |
| Candidate tree | Living mature eucalypt; proposed screening threshold DBH ≥50 cm |
| Orientation | Prefer S–SE or ESE; avoid NW afternoon exposure |

Artificial boxes with 100 mm and 150 mm entrances have been occupied, while about 130 mm may reduce some non-target access. These dimensions are engineering assumptions, not confirmed biological optima.

A 600–900 mm deep variant will test whether additional depth improves temperature stratification and buffering without requiring an impractical 2.5 m structure.

An ESP32 will record temperature, humidity, light and movement. Optional camera/audio AI will classify events as likely glider, native visitor, possible risk visitor or noise. It supports logging and low-risk state selection only. Hidden fins respond to heat or moisture; a low-noise fan runs only when the chamber appears empty or risk is critical. An external indicator reports faults, condensation or unusual occupancy.

## Engineering Targets

HollowHaven will treat thermal buffering, rather than a single “ideal” temperature, as the primary performance goal:

- reduce the afternoon internal temperature peak by at least 4–5°C relative to ambient or a conventional-box control;
- treat approximately 7°C of peak reduction as an aspirational natural-hollow benchmark;
- measure maximum temperature, heating rate, duration of heat exposure and day–night variability;
- retain passive shade, insulation, drainage and ventilation during power or sensor failure.

No defensible preferred humidity range is known. The prototype will record RH, dew point and condensation instead of claiming an unsupported target; persistent moisture triggers maintenance warnings.

## Components

ESP32; SHT31/DHT22 temperature-humidity sensor; PIR/VL53L0X motion sensor; BH1750 light sensor; optional camera/microphone; servo ventilation; low-noise fan; external indicator; USB power; and a modular insulated, drained 3D-printed shell. A field version could add solar charging and deep sleep.

## Testing and Timeline

| Period | Work |
| --- | --- |
| Week 4 | Confirm species, evidence, dimensions and proposal |
| Weeks 5–6 | Build ESP32 sensing and environmental logging |
| Weeks 7–8 | Add ventilation, ambient feedback and AI classification |
| Week 9 | Demonstrate context states and early enclosure |
| Weeks 10–11 | Fabricate and integrate standard and deep variants |
| Weeks 11–12 | Test heat, moisture, power failure and misclassification |
| Week 13 | Complete report, manual and final demonstration |

Testing uses controlled heat, objects and prerecorded media, not wild animals. Success means measurable buffering, safe failure behaviour, reliable sensing and explainable state changes—not proven field suitability.

## Risks and Limitations

- **Ethics:** No testing with wild animals without approval.
- **Ecology:** Artificial hollows supplement rather than replace mature-tree protection.
- **Heat and power:** Active cooling must fail safely without trapping an animal.
- **Multi-species use:** Detection of another native species does not justify automatic exclusion.
- **AI reliability:** Misclassification may trigger logging or human review only.
- **Evidence:** Optimal volume, temperature and humidity remain uncertain.
- **Installation:** Future work at 10–15 m requires ethics, land-manager and arborist approval.

## Project Value

HollowHaven is a testable pervasive-computing prototype using natural hollows as a benchmark while separating evidence, engineering targets and hypotheses. It combines Animal–Computer Interaction, more-than-human design, sensing, cautious AI, physical actuation and custom fabrication.
