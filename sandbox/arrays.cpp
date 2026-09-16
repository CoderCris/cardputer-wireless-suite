// ============================================================================
// Guion de demostración de docs/08-arrays-y-cadenas.md
//
//     g++ -Wall -Wextra -o /tmp/arrays sandbox/arrays.cpp && /tmp/arrays
//
// No es código del firmware: PlatformIO solo compila src/. Sirve para ver con
// tus ojos cada afirmación del documento en lugar de creerla.
//
// Avisa a propósito con -Wall: el warning de sizeof sobre un parámetro array es
// parte de lo que se demuestra.
// ============================================================================

#include <cstdio>
#include <cstring>
#include <cstdint>

void recibe(char a[], const char* etiqueta) {
    printf("  dentro de la funcion (%s): sizeof = %zu\n", etiqueta, sizeof(a));
}

int main() {
    char linea[21] = "hola";

    printf("1) sizeof vs strlen\n");
    printf("  sizeof(linea) = %zu   (capacidad reservada, incluido el hueco del \\0)\n", sizeof(linea));
    printf("  strlen(linea) = %zu   (caracteres hasta el primer \\0)\n", strlen(linea));
    recibe(linea, "char a[]");

    printf("\n2) a[i] es *(a+i)\n");
    printf("  linea[2] = %c   *(linea+2) = %c   2[linea] = %c\n", linea[2], *(linea+2), 2[linea]);

    printf("\n3) resta con uint8_t: 0 - 1\n");
    uint8_t n = 0;
    n--;
    printf("  uint8_t n = 0; n--;  ->  n = %u\n", n);
    uint16_t m = 0; m--;
    printf("  uint16_t m = 0; m--; ->  m = %u\n", m);

    printf("\n4) promocion en expresiones\n");
    uint8_t a = 200, b = 100;
    printf("  uint8_t 200+100 guardado en uint8_t = %u\n", (uint8_t)(a+b));
    printf("  uint8_t 200+100 evaluado en int     = %d\n", a+b);

    printf("\n5) comparar arrays con == compara DIRECCIONES\n");
    char x[8] = "abc", y[8] = "abc";
    printf("  strcmp(x,y) == 0 ? %s   (contenido igual)\n", strcmp(x,y)==0 ? "si" : "no");
    printf("  x == y ?           %s   (direcciones distintas)\n", (void*)x==(void*)y ? "si" : "no");

    printf("\n6) memmove para abrir hueco en el medio\n");
    char buf[21] = "hola mundo";
    size_t len = strlen(buf);
    size_t pos = 4;                                  // insertar en el indice 4
    memmove(buf + pos + 1, buf + pos, len - pos + 1); // +1 arrastra tambien el \0
    buf[pos] = ',';
    printf("  \"hola mundo\" con ',' insertada en 4 -> \"%s\"\n", buf);

    printf("\n7) el \\0 no se copia solo\n");
    char dst[8];
    memset(dst, '#', sizeof(dst));
    memcpy(dst, "abc", 3);                 // copia 3 bytes, sin terminador
    printf("  memcpy 3 bytes y printf %%s -> \"%.8s\" (basura tras 'abc')\n", dst);
    return 0;
}
