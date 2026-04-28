# Testers Parte B

Este directorio deja automatizada la primera parte de la Parte B del `README`:

- validacion funcional basica del servidor
- casos malformados y fragmentacion TCP
- pruebas de estres y cierres abruptos
- pasada opcional con el mismo comando de `valgrind` que pide el proyecto

## Uso

Desde la raiz del repo:

```bash
python3 tests/part_b_testers.py --binary ./ircserv --password pass123
```

Si quieres ejecutar ademas la comprobacion exacta de leaks del `README`:

```bash
python3 tests/part_b_testers.py --binary ./ircserv --password pass123 --use-valgrind
```

Ese modo usa literalmente:

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --track-origins=yes ./ircserv 6667 <password>
```

## Salida

El runner imprime cada caso como `PASS` o `FAIL` y deja logs en:

- `tests/artifacts/server.log`
- `tests/artifacts/valgrind.log` si se ejecuta con `--use-valgrind`

## Casos cubiertos

- registro basico
- registro con comandos fragmentados en varios paquetes
- `JOIN` y `PRIVMSG`
- bonus Bot: comandos directos `!help`, `!ping`, `!time`
- bonus Bot: respuesta en canal y ausencia de respuesta a `NOTICE`
- bonus Bot: nick `Bot` reservado
- bonus DCC SEND: reenvio intacto usuario a usuario
- bonus DCC SEND: mensaje fragmentado y nick inexistente
- control de `TOPIC` desde fuera del canal
- control de `TOPIC` con `+t` para exigir operador
- control de `INVITE` sobre usuarios ya presentes
- flujo completo de invitacion en canales `+i`
- control de modos desconocidos en `MODE`
- roundtrip de `MODE +itkl` con key y limite de usuarios
- proteccion frente a buffer sin `\\r\\n` por encima de 4096 bytes
- linea IRC por encima de 512 bytes
- desconexiones abruptas en masa
- flood de mensajes para verificar que el servidor sigue respondiendo
