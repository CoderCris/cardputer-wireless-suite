# Fases — adaptación del ciclo de pentesting al dominio RF/físico

El grafo que estructura la navegación del [hito 8](../roadmap.md). Parte del ciclo
clásico de pentesting, pero **no lo copia**: copiarlo daría cinco menús vacíos.

---

## Por qué el diagrama clásico no encaja

El ciclo estándar (Pre-Engagement → Information Gathering ↔ Vulnerability
Assessment ↔ Exploitation ↔ Lateral Movement ↔ Post-Exploitation → PoC →
Post-Engagement) está pensado para pentesting de red y host, donde el objetivo es
**ejecutar código en el objetivo**.

Un Cardputer no hace eso. No hay *Exploitation* en ese sentido, ni *Lateral
Movement*, ni *Post-Exploitation*. Vive casi entero en *Information Gathering*,
asoma la cabeza en *Vulnerability Assessment* (WPS activo, gestión frames sin
protección, IR sin rolling code, BLE sin pairing) y tiene una fase propia que el
diagrama clásico no contempla: **replay e inyección de señal física**.

Forzar las siete cajas originales sería cargo cult. Se adaptan a cinco.

## Las cinco fases

| Fase | Qué hace | Hitos que la pueblan | Qué produce |
|------|----------|----------------------|-------------|
| **Sense** | Captura pasiva bruta, sin interpretar | 3 (WiFi), 4 (IR, receptor externo), 5 (BLE), 7 (I2S) | Observaciones crudas |
| **Identify** | Correlación: de capturas a entidades con identidad estable | plataforma | Nodos del grafo |
| **Assess** | Marcar propiedades explotables sobre una entidad | plataforma | Atributos/etiquetas |
| **Interact** | Emisión activa: replay, inyección, advertising | 4 (IR TX), y radio futura | Observaciones nuevas |
| **Evidence** | Exportar algo que se sostenga fuera del dispositivo | 2 (SD) | Ficheros en la MicroSD |

*Sense* es el sniffer escupiendo tramas; no sabe qué son. *Identify* es lo que
convierte "he visto este BSSID en el canal 6" en "existe este AP". *Assess* es
donde una entidad gana la marca de que su WPS está activo. *Interact* es lo único
activo, y por tanto lo único con implicaciones legales. *Evidence* es el
equivalente del *Proof-of-Concept* del diagrama original.

## Las aristas

Las flechas bidireccionales del diagrama clásico sobreviven intactas: **Interact
realimenta a Sense** (emites, vuelves a escuchar, observas el efecto), y *Assess*
puede devolverte a *Sense* pidiendo captura dirigida. El grafo no es una tubería.

```
        ┌──────────────────────────────────────────┐
        │                                          v
     Sense ──> Identify ──> Assess ──> Interact ──┘
        ^                      │           │
        └──────────────────────┘           │
                                           v
                                       Evidence
                                    (desde cualquier fase)
```

## Consecuencia sobre la UI

Esto no es taxonomía decorativa: es lo que hace que el hito 8 sea un navegador y
no un menú. La UI **no lista herramientas, lista nodos**, y ofrece las herramientas
cuyo tipo de entrada casa con el nodo seleccionado. Un nodo de tipo AP con
propiedad "WPS activo" ofrece cosas distintas a un nodo de tipo código IR.

Y una regla de diseño derivada: **la fase Interact se trata distinto en la
interfaz**. Es la única que emite, la única con consecuencias sobre terceros y la
única que puede ser ilegal según el contexto. Que el usuario tenga que cruzar un
límite explícito para entrar en ella es parte del diseño, no una molestia.

## Relación con el enfoque blue team

Cada fase tiene su lectura defensiva: *Sense* es lo que un IDS inalámbrico intenta
detectar en el atacante; *Interact* es lo que deja rastro observable. Construir la
herramienta desde dentro es la forma de entender qué señales genera.
