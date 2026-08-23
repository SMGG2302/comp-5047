# Reciprocal Refuge

## Project discussion outline
### Joe Taylor

We propose to build **Reciprocal Refuge**: a small, solar-powered habitat object that combines a protected hollow for a locally appropriate hollow-dependent animal with an external heat refuge for native pollinators. It senses heat, light, humidity and disturbance, then changes shade, ventilation and access conditions to support the animals using it.

This is not a device that “saves wildlife” through surveillance. It is a modest piece of responsive habitat infrastructure that makes space for non-human needs in an urban or peri-urban landscape.

---

## 1. Ecological situation and design challenge

### The situation

Across Australia, habitat loss, fragmentation, urban heat and extreme weather reduce the availability of stable shelter, nesting cavities, shade, water and food for native species. Mature trees with hollows can take decades or centuries to form, while heat-stressed cities can also be hostile to native bees and other pollinators.

We are responding to this combined condition:

> **How might a small habitat object attend to the changing shelter and thermal needs of native animals and pollinators, rather than merely using them as data for human observation?**

### Proposed non-human participants

The final species must be selected for our intended location and checked with local ecological guidance. For the prototype, we will use these broad participant groups:

- A **locally appropriate hollow-dependent species**, such as a small native bird, microbat, possum, glider, or threatened flying fox.
- **Native pollinators**, especially solitary native bees and other small insects that need shade, water and flowering habitat.
- The surrounding **plant, soil and microbial community**, which forms the ecological setting rather than just decoration.

We will not capture, tag, feed, lure or handle wildlife. The project will be tested with simulated occupancy and environmental conditions. If later installed outdoors, it would require appropriate species-specific dimensions, permissions and ecological advice.

---

## 2. Critical framing

### The thylacine: act before absence

The thylacine is our historical framing, not our project’s target animal. Its extinction reminds us that conservation can arrive too late when action depends on spectacular proof, public attention or a species becoming a symbol of loss.

**Reciprocal Refuge asks us to act while living species still have needs that can be noticed and supported.** We will use this framing carefully: it is not a claim that a single object can repair extinction, but an argument for everyday, anticipatory ecological care.

### More-than-human and multispecies design

The animals are not passive “users” whose behaviour we simply monitor. Their choices are meaningful input:

- An animal **occupying** the hollow suggests that its location and microclimate may be acceptable.
- An animal **avoiding** or leaving the hollow is also feedback; we should not override that refusal with more control or surveillance.
- Pollinator presence, absence and patterns of use can inform later changes to shade, planting and placement.

The system therefore has a limited role: it protects certain conditions, responds to stress and remains open to non-human refusal.

### Designing with Country

We will not describe this project as Country-centred by default. Designing with Country requires meaningful engagement with the relevant Traditional Owners and local knowledge holders.

Our project can begin from Country-aware commitments:

- choose a site and species appropriate to the local place;
- treat land, water, plants, animals and people as connected rather than separate “resources”;
- avoid a universal, transplantable habitat-box solution;
- seek guidance before any real-world deployment.

---

## 3. The object: two connected habitat layers

```
              [ Pollinator heat-refuge canopy ]
        shade petals + shallow wick-water point
                    /               \
       external light / caretaker signal \ solar roof
                  [ insulated hollow ]
       adjustable entrance baffle + ventilation
                    [ root/ground base ]
         sensor channels + removable electronics bay
```

### Inner layer: adaptive hollow

The insulated inner chamber is a dark, sheltered cavity. It responds to temperature, humidity, unwanted light and disturbance by adjusting a small entrance baffle and ventilation.

### Outer layer: pollinator heat refuge

The outer canopy provides shade and a very shallow, stone-filled wick-water point. Servo-operated “petals” open for airflow and close to create shade when heat and bright sun combine. This layer is designed as a refuge, not a feeding system; nearby locally appropriate plants would be the food source.

### Human interaction

The primary interaction is embodied and ambient:

- A caretaker approaches, hears a subtle sound or sees diffuse light, and reads the refuge’s condition through its changing physical form.
- They can open a protected service hatch to refill the passive water reservoir, inspect the electronics and clean the external pollinator surface.
- There is no required web dashboard, phone app or screen.

---

## 4. Grade-safe technical scope

To make the project reliable, we will build **one functional prototype with three ecological states**, rather than claiming to build a universal multi-species habitat.

| State inferred by the system | Example sensor conditions | Physical response |
|---|---|---|
| **Suitable refuge** | Moderate temperature, low light inside, low disturbance | Entrance remains sheltered; canopy is partly open; soft green/neutral light |
| **Heat stress** | High temperature and bright external light | Fan runs; hollow baffle opens for ventilation; pollinator shade petals close/open to increase shade; amber light |
| **Disturbance / unsuitable** | Repeated vibration or loud sound, or unwanted internal light | Entrance baffle shifts toward a more protected position; external light pulses softly for the caretaker |

The core success demonstration will be: **we apply heat/light to the model → the microcontroller infers heat stress → ventilation and shade physically change.** We will also demonstrate one disturbance event.

### Hardware

| Component | Role |
|---|---|
| ESP32 microcontroller | Reads sensors, controls actuators and connects to an AI API over Wi-Fi |
| Temperature/humidity sensor (e.g. DHT22/BME280) | Measures hollow microclimate |
| Light sensor (LDR or BH1750) | Detects harsh external light and unwanted internal light |
| Piezo vibration sensor or accelerometer | Detects disturbance to the enclosure |
| Optional microphone/sound-level sensor | Adds a simple disturbance measure; no species identification required |
| Servo motors | Move hollow baffle and canopy shade petals |
| Small 5 V fan | Provides heat-response ventilation |
| Diffused RGB LED + piezo buzzer | Ambient, non-screen feedback for the human caretaker |
| Solar panel / rechargeable battery or USB power bank | Supports the solar-powered concept; mains/USB power remains acceptable for the studio demonstration |

### AI API: deliberately modest and testable

The project will use an AI API to interpret a short bundle of environmental sensor readings and return one of our three named states: `suitable`, `heat_stress`, or `disturbance`.

The AI does **not** identify animal species or claim to know an animal’s internal state. It assists with contextual interpretation. The ESP32 will retain simple local threshold rules as a backup, so the essential protective response works even if Wi-Fi or the API is unavailable.

We will log and test example inputs before presenting:

```text
temperature: 35 C, humidity: 24%, external light: high, vibration: low
→ heat_stress → fan on, baffle adjusted, shade petals deployed
```

---

## 5. Fabrication and enclosure plan

We will design the enclosure ourselves in Fusion 360 (or comparable CAD software), document iterations, and fabricate it through a combination of 3D printing and laser cutting.

### Form and material rationale

- **Form:** a vertical hollow/log or branching form, with a sheltered core and a flower/tussock-like canopy.
- **Cork or timber outer layer:** evokes bark and provides insulation/texture.
- **Laser-cut recycled plywood ribs:** create a visible root or branch structure.
- **3D-printed recycled PETG/PLA components:** provide weather-resistant sensor mounts, baffle pivots and shade petals.
- **Translucent material:** lets status light glow softly through the “bark” rather than becoming a bright visual display.
- **Removable electronics bay:** protects electronics and makes repair possible without dismantling the habitat structure.

The enclosure should communicate that it belongs near vegetation and is encountered primarily by animals, insects and caretakers—not as a decorative smart-home object.

---

## 6. Rubric checklist

| Rubric requirement | How Reciprocal Refuge addresses it |
|---|---|
| At least one microcontroller | ESP32 |
| At least two sensor types | Temperature/humidity, light, vibration; optional sound level |
| Non-screen feedback | Servo movement, fan, ambient LED and sound |
| Physical, embodied or ambient interaction | The changing hollow aperture and shade canopy are the main interface |
| Context sensing | Microclimate, brightness and disturbance |
| Fabricated enclosure | Team-designed CAD enclosure, 3D printed and laser cut |
| Australian ecological situation and named actors | Fragmented habitat and heat stress affecting local hollow-dependent animals and native pollinators |
| At least one working feature | Heat/light → inferred heat stress → fan and shade/baffle movement |
| Data used for inference, not display alone | Readings combine into ecological states rather than appearing as raw values |
| Adaptive behaviour | Ventilation, entrance protection and shade change by state |
| Response to non-human needs | Darkness, thermal stability, shelter, reduced disturbance, shade and water access |
| AI API | Environmental-pattern interpretation to classify the three refuge states |
| Ecological enclosure form/material | Hollow, bark, root and flower/tussock references; insulating and repairable materials |
| All proposed functions work | We limit the build to three states and test each actuator independently and together |
| Robustness for intended human operator | Protected wiring, low-voltage components, sealed/replaceable sensor modules and accessible service hatch |
| Team-designed/fabricated enclosure | CAD files, fabrication tests and final enclosure will be documented |
| Novelty | One object links hollow microclimate and pollinator heat refuge while treating animal refusal as feedback |
| More-than-human framing | The system protects animal-defined habitat conditions and does not reduce participation to data capture |
| Novel/elegant response | The enclosure itself is the habitat, sensor platform, actuator and ambient ecological signal |
| Creative components/materials | Moving bark/baffle, flower-like shade petals, root channels and diffuse light |
| Clear thinking about site/presence | A small, repairable habitat object for a specific local site, encountered differently by animals, pollinators and caretakers |

---

## 7. Constraints and ethical limits

- We should not promise that an artificial hollow will be adopted by a target species.
- We should not use the system to attract animals, collect sensitive location data or intervene in wildlife without permissions.
- We should not claim an AI API can know an animal’s needs with certainty.
- The prototype demonstrates a **care relationship and adaptive habitat condition**, not a validated conservation intervention.
- If we use Indigenous concepts or a specific Country, we must cite and engage appropriate local knowledge rather than applying the language decoratively.

---

## 8. Proposed next decisions for the group

1. Select our intended local site and nominate one realistic hollow-dependent target species.
2. Decide whether the pollinator layer targets native bees specifically or remains a broader insect heat refuge.
3. Confirm the three final states and remove any feature we cannot test.
4. Prototype the baffle and shade-petal mechanisms before finalising the enclosure.
5. Identify local ecological and, where appropriate, First Nations guidance for species, materials and placement.

## Working title

**Reciprocal Refuge: Acting Before Absence**

*An adaptive hollow and heat refuge for local native wildlife and pollinators.*
