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

## Decisiones abiertas — son del usuario, no se rellenan aquí

### 1. ¿El nodo primitivo es la entidad o la observación?

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

Hasta que estas dos estén decididas, el resto del store no se puede especificar.

---

## Relación con las fases

Cada fase de [`phases.md`](phases.md) produce y consume tipos distintos:
*Sense* solo escribe, *Identify* lee observaciones y escribe entidades, *Assess*
anota atributos, *Interact* escribe observaciones nuevas, *Evidence* solo lee.
El tipo de nodo seleccionado es lo que determina qué herramientas ofrece la UI.
