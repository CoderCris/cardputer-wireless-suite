# Deuda conceptual

Conceptos que has decidido **aparcar conscientemente** para no romper el ritmo de
un hito. No son cosas que no entiendas: son cosas que has elegido no profundizar
*todavía*.

La regla es la de la [escalera](02-abstraction-ladder.md) aplicada a las ideas:
cada entrada tiene un **disparador** — la situación concreta en la que dejará de
ser un rodeo y pasará a ser el camino corto. Cuando llegue esa situación, se paga
la deuda. Antes no.

Formato de cada entrada: qué es en dos frases, dónde te lo encontraste, cuál es el
disparador, y por dónde empezar cuando toque.

---

## Polimorfismo en C++ (`virtual` / `override` / `std::unique_ptr`)

**Aparcado en**: hito 1, leyendo el driver del teclado.

**Qué es, en dos frases.** Un mecanismo para que un mismo trozo de código trabaje
con varias implementaciones distintas de la misma idea sin saber cuál tiene
delante. Se declara una clase base con las operaciones que todas comparten
(`virtual`), cada implementación las reescribe (`override`), y quien las usa habla
solo con la base — la elección de cuál se construye ocurre en un único sitio.

**Dónde te lo encontraste.** `Keyboard_Class::begin()` consulta `M5.getBoard()` y
construye un `IOMatrixKeyboardReader` (matriz de GPIO, Cardputer V1) o un
`TCA8418KeyboardReader` (chip I2C, Cardputer ADV). El resto del driver llama a
`_keyboard_reader->update()` sin enterarse de cuál de los dos es. Ficheros:
`utility/Keyboard/KeyboardReader/KeyboardReader.h` (la base) y `Keyboard.cpp`
(la elección).

**Disparador para retomarlo.** Cuando aparezca la primera situación en la que
**dos cosas distintas tengan que comportarse igual desde fuera**. En el roadmap
eso está previsto en dos puntos:

- **Hito 2**, al entrar en vigor el [contrato de salida](06-model/data-model.md):
  varias herramientas emitiendo registros tipados al mismo store sin conocerse
  entre ellas. Ese es exactamente el problema que el polimorfismo resuelve.
- **Hito 8**, en el navegador: ofrecer las herramientas cuyo tipo de entrada casa
  con el nodo seleccionado ([fases](06-model/phases.md)).

**Por dónde empezar cuando toque.** No por la teoría: por reescribir el ejemplo
que ya tienes delante. Coge `KeyboardReader` y quítale el `virtual`; compila y
observa qué deja de funcionar y por qué. Después de ver el fallo, la palabra clave
se explica sola.

**Antes de eso, lo mínimo que sí necesitas saber.** Que `_keyboard_reader->update()`
no llama a una función fija: decide **en tiempo de ejecución** cuál ejecutar,
según el objeto que se construyó. Eso, de momento, basta para leer el driver.
