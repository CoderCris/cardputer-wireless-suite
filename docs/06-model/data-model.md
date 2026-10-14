# Modelo de datos compartido (store)

**El único documento de la capa de visión con efecto inmediato sobre el código.**
Todo lo demás de [`05-vision.md`](../05-vision.md) puede esperar; esto no, porque
condiciona el hito 2 y a partir de ahí todos.

---

## El contrato

> Ninguna herramienta imprime a pantalla como salida primaria. Toda herramienta
> emite **registros tipados** a un almacén común. La pantalla es un consumidor de
> ese almacén, como lo será la SD, el exportador o el coprocesador de display.

Son pocas líneas de diferencia al escribir el hito 2, y son la diferencia entre
que el hito 8 sea un navegador o una reescritura completa.

El síntoma de estar violándolo: una función que captura algo y llama a
`M5.Display.printf()` dentro del bucle de captura. Ahí la información ya se ha
perdido — solo queda su representación.

## La restricción dura: RAM

El grafo en RAM compite con los buffers del stack WiFi en modo promiscuo, que no
son pequeños. Y **no hay PSRAM**: el StampS3 monta un ESP32-S3FN8 y el eFuse
`PSRAM_CAP = None` lo confirma (ver
[`01-hardware/cardputer-map.md`](../01-hardware/cardputer-map.md)). El diseño
parte, por tanto, del segundo escenario de esta tabla, que se conserva para
mostrar por qué la diferencia cambia el diseño entero:

| Escenario | Diseño posible |
|-----------|----------------|
| Con PSRAM (8 MB) | Grafo dinámico, nodos asignados en heap |
| Sin PSRAM (512 KB SRAM, compartidos) | Pool estático de nodos de tamaño fijo, aristas **por índice, nunca por puntero**, y la SD como almacén real |

El segundo escenario es además el mejor pedagógicamente: obliga a razonar sobre
fragmentación y sobre presupuesto de memoria, y se parece más a cómo funciona la
forensia real.

**Micro-tarea previa a cualquier decisión**: imprimir `ESP.getFreeHeap()` al
arrancar y anotarlo en el mapa de hardware: es el presupuesto real del que parte
el store.

## Persistencia: log append-only

Sobre la MicroSD, el almacén real es un **log de solo-anexar**: cada registro se
escribe al final, nunca se modifica lo ya escrito, y el índice en RAM se
**reconstruye leyendo el log al arrancar**. Ventajas concretas: escritura barata
(no hay reordenación), robustez ante corte de alimentación (como mucho pierdes el
último registro), y trazabilidad — el log *es* la evidencia de la fase *Evidence*.

El formato (texto tipo JSONL vs. binario TLV) se decide junto con los campos, y es
un compromiso entre legibilidad fuera del dispositivo y bytes por registro.

---

## Decisiones del usuario

### 1. ¿El nodo primitivo es la entidad o la observación?

**Decidido (2026-10-14): la observación**, siguiendo event sourcing. El log solo
contiene eventos inmutables. Las entidades ("este AP", "este dispositivo") son
*proyecciones*: estado derivado que se reconstruye recorriendo el log, y el índice
en RAM es una de ellas. Una atribución equivocada, como dos MAC randomizadas que
resultan ser el mismo dispositivo, se corrige con una proyección nueva, sin tocar
la evidencia. El coste es que el log crece sin cota y reconstruir el estado al
arrancar se encarece. El remedio habitual son los snapshots, que se aplazan hasta
que el coste se pueda medir.

Lo que sigue es la pregunta original, que se conserva como contexto.

¿Un BSSID visto tres veces en tres canales es **un nodo que se actualiza**, o son
**tres eventos inmutables** de los que una capa superior deriva la entidad?

Para razonarlo: piensa qué pasa con un dispositivo que randomiza su MAC en cada
uno de los dos modelos, y qué implica cada opción para un log que solo sabe
anexar.

### 2. ¿Qué campo lleva obligatoriamente **todo** registro?

Sea cual sea la fuente — sniffer WiFi, decodificador IR, scanner BLE — el
navegador va a necesitar hacer dos cosas con registros heterogéneos: **ordenarlos**
y **correlacionarlos**.

Detalle incómodo del hardware que condiciona la respuesta: el Cardputer **no tiene
RTC con batería**. Al arrancar no sabe qué hora es.

**Decidido (2026-10-14): contador de arranques + tiempo desde el arranque.** El
par `(boot, t)` ordena los eventos de un mismo dispositivo aunque no haya hora de
pared. Antes de escribir el primer registro hay que resolver tres cosas:

1. **Dónde vive el contador.** Para sobrevivir a un apagado tiene que estar en
   memoria no volátil (NVS en la flash, o leerse del último registro de la SD).
   ¿Qué pasa si arrancas sin tarjeta?
2. **Empates.** ¿Qué ocurre con dos eventos en el mismo milisegundo?
3. **Desbordamiento.** `millis()` es un `uint32_t` en ms. ¿Cuántos días tarda en
   dar la vuelta, y lo tolera una sesión de captura larga?

Correlacionar con el mundo exterior (otro dispositivo, un log del SIEM) exige
hora real, y esto no la da. Con event sourcing no hace falta tenerla al
capturar: cuando llegue una referencia (NTP, GPS, introducida a mano), basta con
anexar un evento que diga "el `t` X del boot N corresponde a esta UTC".

Faltan aún los campos restantes y el formato del log (JSONL o TLV).

---

## Relación con las fases

Cada fase de [`phases.md`](phases.md) produce y consume tipos distintos:
*Sense* solo escribe, *Identify* lee observaciones y escribe entidades, *Assess*
anota atributos, *Interact* escribe observaciones nuevas, *Evidence* solo lee.
El tipo de nodo seleccionado es lo que determina qué herramientas ofrece la UI.

---

## Para profundizar

Lecturas para el modelo de datos, en orden recomendado. Todas en inglés.

1. Jay Kreps, *The Log: What every software engineer should know about real-time
   data's unifying abstraction* (blog de ingeniería de LinkedIn, 2013). El log
   append-only como la abstracción central.
2. Martin Fowler, *Event Sourcing* (martinfowler.com, 2005). Vocabulario básico:
   evento, proyección, reconstrucción.
3. Martin Kleppmann, *Designing Data-Intensive Applications* (O'Reilly). Cap. 3
   (almacenamiento en log, índices, compactación) y cap. 11 (event sourcing y
   streams).
4. Martin Kleppmann, *Turning the database inside-out* (charla, Strange Loop
   2014).
5. Leslie Lamport, *Time, Clocks, and the Ordering of Events in a Distributed
   System* (CACM, 1978). El contador de arranques es una versión de su reloj
   lógico. Las clases de Kleppmann *Distributed Systems* (Cambridge, 2020, en
   YouTube) lo explican en vídeo, en las sesiones sobre tiempo y relojes lógicos.
6. Hadley Wickham, *Tidy Data* (Journal of Statistical Software, 2014). Cómo dar
   forma a las observaciones para que se puedan analizar.
7. John Lambert, *Defenders think in lists. Attackers think in graphs.* (2015).
   Por qué el store es un grafo.
8. Ralph Kimball, *The Data Warehouse Toolkit*, capítulos de tablas de hechos y
   dimensiones (opcional).
9. Código del Almirantazgo (NATO, AJP-2.1): fiabilidad de la fuente (A-F) y
   credibilidad de la información (1-6), para la valoración de la información.
