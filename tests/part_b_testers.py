#!/usr/bin/env python3
import argparse
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import time


ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ARTIFACTS_DIR = os.path.join(ROOT_DIR, "tests", "artifacts")


class TestFailure(Exception):
    pass


class IrcClient(object):
    def __init__(self, host, port, nick, password, timeout=1.0):
        self.host = host
        self.port = port
        self.nick = nick
        self.password = password
        self.timeout = timeout
        self.sock = None

    def connect(self):
        self.sock = socket.create_connection((self.host, self.port), self.timeout)
        self.sock.settimeout(0.2)

    def close(self):
        if self.sock is None:
            return
        try:
            self.send_line("QUIT :test teardown")
        except Exception:
            pass
        try:
            self.sock.close()
        except Exception:
            pass
        self.sock = None

    def send_raw(self, payload):
        self.sock.sendall(payload)

    def send_line(self, line):
        self.send_raw((line + "\r\n").encode("utf-8"))

    def recv_available(self, delay=0.05):
        time.sleep(delay)
        chunks = []
        while True:
            try:
                chunk = self.sock.recv(65536)
            except socket.timeout:
                break
            if not chunk:
                break
            chunks.append(chunk)
        return b"".join(chunks).decode("utf-8", "replace")

    def register(self):
        self.send_line("PASS " + self.password)
        self.send_line("NICK " + self.nick)
        self.send_line("USER {0} 0 * :{0}".format(self.nick))
        reply = self.recv_available(0.1)
        if " 001 " not in reply or self.nick not in reply:
            raise TestFailure(
                "registro fallido para {0!r}. Respuesta: {1!r}".format(
                    self.nick, reply
                )
            )
        return reply


class ServerRunner(object):
    def __init__(self, binary, port, password, use_valgrind):
        self.binary = binary
        self.port = port
        self.password = password
        self.use_valgrind = use_valgrind
        self.process = None
        self.streams = []
        self.server_log_path = os.path.join(ARTIFACTS_DIR, "server.log")
        self.valgrind_log_path = os.path.join(ARTIFACTS_DIR, "valgrind.log")

    def start(self):
        ensure_artifacts_dir()
        for path in (self.server_log_path, self.valgrind_log_path):
            try:
                os.remove(path)
            except OSError:
                pass

        command = []
        stderr_target = open(self.server_log_path, "w")
        if self.use_valgrind:
            if shutil.which("valgrind") is None:
                stderr_target.close()
                raise RuntimeError(
                    "valgrind no esta instalado en este entorno; no puedo ejecutar "
                    "el comando exacto del README."
                )
            command = [
                "valgrind",
                "--leak-check=full",
                "--show-leak-kinds=all",
                "--track-fds=yes",
                "--track-origins=yes",
                "--log-file=" + self.valgrind_log_path,
                self.binary,
                str(self.port),
                self.password,
            ]
        else:
            command = [self.binary, str(self.port), self.password]

        stdout_target = open(self.server_log_path, "a")
        self.streams = [stdout_target, stderr_target]
        self.process = subprocess.Popen(
            command,
            cwd=ROOT_DIR,
            stdout=stdout_target,
            stderr=stderr_target,
        )
        wait_for_port("127.0.0.1", self.port, timeout=10.0)

    def stop(self):
        if self.process is None:
            return
        if self.process.poll() is None:
            self.process.send_signal(signal.SIGINT)
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)
        for stream in self.streams:
            try:
                stream.close()
            except Exception:
                pass
        self.streams = []
        self.process = None

    def read_server_log(self):
        return read_text_file(self.server_log_path)

    def read_valgrind_log(self):
        return read_text_file(self.valgrind_log_path)


def ensure_artifacts_dir():
    if not os.path.isdir(ARTIFACTS_DIR):
        os.makedirs(ARTIFACTS_DIR)


def read_text_file(path):
    if not os.path.exists(path):
        return ""
    handle = open(path, "r")
    try:
        return handle.read()
    finally:
        handle.close()


def wait_for_port(host, port, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        probe.settimeout(0.2)
        try:
            probe.connect((host, port))
            probe.close()
            return
        except Exception:
            probe.close()
            time.sleep(0.1)
    raise RuntimeError("el servidor no abrio el puerto {0}".format(port))


def assert_contains(haystack, needle, label):
    if needle not in haystack:
        raise TestFailure(
            "{0}: esperaba encontrar {1!r} en {2!r}".format(label, needle, haystack)
        )


def assert_not_contains(haystack, needle, label):
    if needle in haystack:
        raise TestFailure(
            "{0}: no esperaba encontrar {1!r} en {2!r}".format(label, needle, haystack)
        )


def make_client(port, nick, password):
    client = IrcClient("127.0.0.1", port, nick, password)
    client.connect()
    return client


def test_basic_registration(port, password):
    client = make_client(port, "basic1", password)
    try:
        reply = client.register()
        assert_contains(reply, " 001 ", "registro basico")
    finally:
        client.close()


def test_fragmented_registration(port, password):
    client = make_client(port, "fragx1", password)
    try:
        fragments = [
            b"PA",
            ("SS {0}\r".format(password)).encode("utf-8"),
            b"\nNI",
            b"CK fragx1\r\nUS",
            b"ER fragx1 0 * :Fragmented\r\n",
        ]
        for fragment in fragments:
            client.send_raw(fragment)
            time.sleep(0.03)
        reply = client.recv_available(0.15)
        assert_contains(reply, " 001 ", "fragmentacion TCP")
    finally:
        client.close()


def test_join_and_privmsg(port, password):
    alice = make_client(port, "joina1", password)
    bob = make_client(port, "joinb1", password)
    try:
        alice.register()
        bob.register()

        channel = "#flowa1"
        alice.send_line("JOIN " + channel)
        alice_reply = alice.recv_available(0.1)
        assert_contains(alice_reply, " 353 ", "JOIN primer usuario")

        bob.send_line("JOIN " + channel)
        bob_reply = bob.recv_available(0.1)
        assert_contains(bob_reply, " 353 ", "JOIN segundo usuario")
        assert_contains(alice.recv_available(0.1), " JOIN " + channel, "broadcast JOIN")

        alice.send_line("PRIVMSG " + channel + " :hola testers")
        bob_msg = bob.recv_available(0.1)
        assert_contains(bob_msg, "PRIVMSG " + channel + " :hola testers", "PRIVMSG canal")
    finally:
        alice.close()
        bob.close()


def test_bot_direct_help(port, password):
    client = make_client(port, "bothelp1", password)
    try:
        client.register()
        client.send_line("PRIVMSG Bot :!help")
        reply = client.recv_available(0.1)
        assert_contains(reply, "NOTICE bothelp1", "Bot !help directo")
        assert_contains(reply, "!ping", "Bot !help lista comandos")
    finally:
        client.close()


def test_bot_direct_ping(port, password):
    client = make_client(port, "botping1", password)
    try:
        client.register()
        client.send_line("PRIVMSG Bot :!ping")
        reply = client.recv_available(0.1)
        assert_contains(
            reply,
            ":Bot!bot@ircserv NOTICE botping1 :pong",
            "Bot !ping directo",
        )
    finally:
        client.close()


def test_bot_direct_time(port, password):
    client = make_client(port, "bottime1", password)
    try:
        client.register()
        client.send_line("PRIVMSG Bot :!time")
        reply = client.recv_available(0.1)
        if re.search(
            r":Bot!bot@ircserv NOTICE bottime1 :time "
            r"\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}",
            reply,
        ) is None:
            raise TestFailure("Bot !time no devolvio formato estable: {0!r}".format(reply))
    finally:
        client.close()


def test_bot_channel_ping(port, password):
    client = make_client(port, "botchan1", password)
    try:
        client.register()
        channel = "#botchan"
        client.send_line("JOIN " + channel)
        client.recv_available(0.1)

        client.send_line("PRIVMSG " + channel + " :!ping")
        reply = client.recv_available(0.1)
        assert_contains(
            reply,
            ":Bot!bot@ircserv NOTICE " + channel + " :pong",
            "Bot !ping canal",
        )
    finally:
        client.close()


def test_bot_ignores_notice(port, password):
    client = make_client(port, "botnot1", password)
    try:
        client.register()
        channel = "#botnot"
        client.send_line("JOIN " + channel)
        client.recv_available(0.1)

        client.send_line("NOTICE " + channel + " :!ping")
        reply = client.recv_available(0.1)
        assert_not_contains(
            reply,
            ":Bot!bot@ircserv NOTICE",
            "Bot no debe reaccionar a NOTICE",
        )
    finally:
        client.close()


def test_bot_nickname_is_reserved(port, password):
    client = make_client(port, "unused", password)
    try:
        client.send_line("PASS " + password)
        client.send_line("NICK Bot")
        reply = client.recv_available(0.1)
        assert_contains(reply, " 433 ", "NICK Bot reservado")
    finally:
        client.close()


def test_dcc_send_user_to_user(port, password):
    sender = make_client(port, "dccsend1", password)
    receiver = make_client(port, "dccrecv1", password)
    try:
        sender.register()
        receiver.register()

        payload = "\x01DCC SEND test.txt 2130706433 5000 42\x01"
        sender.send_line("PRIVMSG dccrecv1 :" + payload)
        reply = receiver.recv_available(0.1)
        assert_contains(reply, "PRIVMSG dccrecv1 :" + payload, "DCC SEND intacto")
        assert_contains(reply, ":dccsend1!", "DCC SEND conserva prefix")
    finally:
        sender.close()
        receiver.close()


def test_dcc_send_fragmented(port, password):
    sender = make_client(port, "dccfrag1", password)
    receiver = make_client(port, "dccrecv2", password)
    try:
        sender.register()
        receiver.register()

        raw = "PRIVMSG dccrecv2 :\x01DCC SEND frag.bin 2130706433 5001 84\x01\r\n"
        fragments = [
            raw[:8].encode("utf-8"),
            raw[8:24].encode("utf-8"),
            raw[24:45].encode("utf-8"),
            raw[45:].encode("utf-8"),
        ]
        for fragment in fragments:
            sender.send_raw(fragment)
            time.sleep(0.03)
        reply = receiver.recv_available(0.15)
        assert_contains(
            reply,
            ":\x01DCC SEND frag.bin 2130706433 5001 84\x01",
            "DCC SEND fragmentado",
        )
    finally:
        sender.close()
        receiver.close()


def test_dcc_send_to_missing_user(port, password):
    sender = make_client(port, "dccmiss1", password)
    try:
        sender.register()
        sender.send_line("PRIVMSG missing :\x01DCC SEND test.txt 1 2 3\x01")
        reply = sender.recv_available(0.1)
        assert_contains(reply, " 401 ", "DCC SEND a nick inexistente")
    finally:
        sender.close()


def test_topic_requires_membership(port, password):
    op = make_client(port, "topicop", password)
    outsider = make_client(port, "topiczz", password)
    try:
        op.register()
        outsider.register()

        channel = "#topic1"
        op.send_line("JOIN " + channel)
        op.recv_available(0.1)

        outsider.send_line("TOPIC " + channel + " :intrusion")
        outsider_reply = outsider.recv_available(0.1)
        assert_contains(
            outsider_reply,
            " 442 ",
            "TOPIC debe exigir pertenencia al canal",
        )
        assert_not_contains(
            op.recv_available(0.1),
            " TOPIC " + channel + " :intrusion",
            "TOPIC outsider no debe propagarse",
        )
    finally:
        op.close()
        outsider.close()


def test_invite_rejects_existing_member(port, password):
    op = make_client(port, "inviteop", password)
    member = make_client(port, "invitem1", password)
    try:
        op.register()
        member.register()

        channel = "#invite1"
        op.send_line("JOIN " + channel)
        op.recv_available(0.1)
        member.send_line("JOIN " + channel)
        member.recv_available(0.1)
        op.recv_available(0.1)

        op.send_line("INVITE invitem1 " + channel)
        reply = op.recv_available(0.1)
        assert_contains(
            reply,
            " 443 ",
            "INVITE debe rechazar usuarios ya presentes en el canal",
        )
    finally:
        op.close()
        member.close()


def test_unknown_mode_returns_472(port, password):
    op = make_client(port, "modeop1", password)
    try:
        op.register()
        channel = "#modez1"
        op.send_line("JOIN " + channel)
        op.recv_available(0.1)

        op.send_line("MODE " + channel + " +z")
        reply = op.recv_available(0.1)
        assert_contains(reply, " 472 ", "MODE desconocido")
        assert_not_contains(reply, " MODE " + channel + " +", "MODE desconocido")
    finally:
        op.close()


def test_topic_restricted_requires_operator(port, password):
    op = make_client(port, "topop2", password)
    member = make_client(port, "topme2", password)
    try:
        op.register()
        member.register()

        channel = "#topict2"
        op.send_line("JOIN " + channel)
        op.recv_available(0.1)
        member.send_line("JOIN " + channel)
        member.recv_available(0.1)
        op.recv_available(0.1)

        op.send_line("MODE " + channel + " +t")
        op.recv_available(0.1)
        member.recv_available(0.1)

        member.send_line("TOPIC " + channel + " :bloqueado")
        reply = member.recv_available(0.1)
        assert_contains(reply, " 482 ", "TOPIC con +t requiere operador")
        assert_not_contains(
            op.recv_available(0.1),
            " TOPIC " + channel + " :bloqueado",
            "TOPIC bloqueado no debe propagarse",
        )

        op.send_line("TOPIC " + channel + " :permitido")
        op_reply = op.recv_available(0.1)
        member_reply = member.recv_available(0.1)
        assert_contains(op_reply, " TOPIC " + channel + " :permitido", "TOPIC op")
        assert_contains(
            member_reply, " TOPIC " + channel + " :permitido", "TOPIC propagado"
        )
    finally:
        op.close()
        member.close()


def test_invite_only_join_flow(port, password):
    op = make_client(port, "invop2", password)
    guest = make_client(port, "invgst2", password)
    try:
        op.register()
        guest.register()

        channel = "#invonl2"
        op.send_line("JOIN " + channel)
        op.recv_available(0.1)

        op.send_line("MODE " + channel + " +i")
        op.recv_available(0.1)

        guest.send_line("JOIN " + channel)
        denied = guest.recv_available(0.1)
        assert_contains(denied, " 473 ", "JOIN en canal +i sin invitacion")

        op.send_line("INVITE invgst2 " + channel)
        invite_ack = op.recv_available(0.1)
        invite_msg = guest.recv_available(0.1)
        assert_contains(invite_ack, " 341 ", "ACK INVITE")
        assert_contains(invite_msg, " INVITE invgst2 :" + channel, "mensaje INVITE")

        guest.send_line("JOIN " + channel)
        joined = guest.recv_available(0.1)
        assert_contains(joined, " JOIN " + channel, "JOIN tras invitacion")
    finally:
        op.close()
        guest.close()


def test_mode_key_and_limit_flow(port, password):
    op = make_client(port, "modok1", password)
    guest1 = make_client(port, "modg1a", password)
    guest2 = make_client(port, "modg2a", password)
    try:
        op.register()
        guest1.register()
        guest2.register()

        channel = "#modeok2"
        op.send_line("JOIN " + channel)
        op.recv_available(0.1)

        op.send_line("MODE " + channel + " +itkl secret 2")
        mode_reply = op.recv_available(0.1)
        assert_contains(mode_reply, " MODE " + channel + " +itkl secret 2", "MODE set")

        op.send_line("MODE " + channel)
        mode_query = op.recv_available(0.1)
        assert_contains(mode_query, " 324 ", "MODE query")
        assert_contains(mode_query, channel + " +itkl secret 2", "MODE query flags")

        op.send_line("INVITE modg1a " + channel)
        op.recv_available(0.1)
        guest1.recv_available(0.1)

        op.send_line("INVITE modg2a " + channel)
        op.recv_available(0.1)
        guest2.recv_available(0.1)

        guest1.send_line("JOIN " + channel)
        assert_contains(
            guest1.recv_available(0.1), " 475 ", "JOIN sin key debe fallar con +k"
        )

        guest1.send_line("JOIN " + channel + " secret")
        assert_contains(
            guest1.recv_available(0.1), " JOIN " + channel, "JOIN con key correcta"
        )

        guest2.send_line("JOIN " + channel + " secret")
        assert_contains(
            guest2.recv_available(0.1), " 471 ", "JOIN debe fallar al superar +l"
        )
    finally:
        op.close()
        guest1.close()
        guest2.close()


def test_buffer_overflow_disconnect(port, password):
    bad = make_client(port, "buffx1", password)
    probe = make_client(port, "buffok", password)
    try:
        bad.register()
        bad.send_raw(b"A" * 5000)
        time.sleep(0.2)

        disconnected = False
        try:
            data = bad.sock.recv(1024)
            disconnected = (data == b"")
        except socket.error:
            disconnected = True

        if not disconnected:
            raise TestFailure(
                "el cliente sobredimensionado no fue desconectado tras superar 4096 bytes"
            )

        probe.register()
        probe.send_line("PING :alive")
        assert_contains(probe.recv_available(0.1), " PONG ", "liveness tras overflow")
    finally:
        bad.close()
        probe.close()


def test_mass_disconnect(port, password):
    clients = []
    probe = None
    try:
        for idx in range(20):
            nick = "mass{0:02d}".format(idx)
            client = make_client(port, nick, password)
            client.register()
            clients.append(client)

        for client in clients:
            client.sock.close()
            client.sock = None

        time.sleep(0.4)

        probe = make_client(port, "massok", password)
        probe.register()
        probe.send_line("PING :still-up")
        assert_contains(
            probe.recv_available(0.1),
            " PONG ",
            "liveness tras cierres abruptos",
        )
    finally:
        for client in clients:
            client.close()
        if probe is not None:
            probe.close()


def test_overlong_command_stability(port, password):
    sender = make_client(port, "longs1", password)
    target = make_client(port, "longt1", password)
    probe = make_client(port, "longp1", password)
    try:
        sender.register()
        target.register()

        payload = "x" * 700
        sender.send_line("PRIVMSG longt1 :" + payload)
        delivered = target.recv_available(0.1)
        if not delivered:
            raise TestFailure("el mensaje sobredimensionado no produjo ninguna respuesta")

        probe.register()
        probe.send_line("PING :oversized-ok")
        assert_contains(probe.recv_available(0.1), " PONG ", "liveness tras linea > 512")
    finally:
        sender.close()
        target.close()
        probe.close()


def test_message_flood_liveness(port, password):
    receiver = make_client(port, "sinkx1", password)
    senders = []
    probe = None
    try:
        receiver.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
        receiver.register()

        for idx in range(10):
            nick = "flood{0}".format(idx)
            sender = make_client(port, nick, password)
            sender.register()
            senders.append(sender)

        payload = "x" * 380
        for round_index in range(250):
            for sender in senders:
                sender.send_line(
                    "PRIVMSG sinkx1 :{0}-{1:04d}".format(payload, round_index)
                )

        time.sleep(0.4)
        probe = make_client(port, "floodok", password)
        probe.register()
        probe.send_line("PING :flood-ok")
        assert_contains(
            probe.recv_available(0.2),
            " PONG ",
            "liveness tras flood de mensajes",
        )
    finally:
        receiver.close()
        for sender in senders:
            sender.close()
        if probe is not None:
            probe.close()


def run_valgrind_smoke(port, password):
    client = make_client(port, "vgsmok1", password)
    try:
        client.register()
        client.send_line("JOIN #vgchan")
        client.recv_available(0.1)
        client.send_line("PING :vg")
        assert_contains(client.recv_available(0.1), " PONG ", "smoke valgrind")
    finally:
        client.close()


def summarize_valgrind(log_text):
    interesting = []
    for line in log_text.splitlines():
        if (
            "FILE DESCRIPTORS" in line
            or "HEAP SUMMARY" in line
            or "in use at exit" in line
            or "total heap usage" in line
            or "All heap blocks were freed" in line
            or "lost:" in line
            or "ERROR SUMMARY" in line
        ):
            interesting.append(line.strip())
    return "\n".join(interesting).strip()


def run_test_case(name, callback, failures):
    try:
        callback()
        print("[PASS] " + name)
    except TestFailure as exc:
        print("[FAIL] " + name + " -> " + str(exc))
        failures.append((name, str(exc)))
    sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser(
        description="Tester automatizado para la parte B de ft_irc"
    )
    parser.add_argument("--binary", default="./ircserv")
    parser.add_argument("--port", type=int, default=6667)
    parser.add_argument("--password", default="pass123")
    parser.add_argument("--use-valgrind", action="store_true")
    args = parser.parse_args()

    binary = args.binary
    if not os.path.isabs(binary):
        binary = os.path.abspath(os.path.join(ROOT_DIR, binary))

    if not os.path.exists(binary):
        print("No existe el binario: " + binary)
        return 2

    failures = []

    runner = ServerRunner(binary, args.port, args.password, False)
    try:
        runner.start()
        tests = [
            ("Registro basico", lambda: test_basic_registration(args.port, args.password)),
            (
                "Fragmentacion TCP en registro",
                lambda: test_fragmented_registration(args.port, args.password),
            ),
            ("JOIN + PRIVMSG", lambda: test_join_and_privmsg(args.port, args.password)),
            ("Bot responde !help directo", lambda: test_bot_direct_help(args.port, args.password)),
            ("Bot responde !ping directo", lambda: test_bot_direct_ping(args.port, args.password)),
            ("Bot responde !time directo", lambda: test_bot_direct_time(args.port, args.password)),
            ("Bot responde !ping en canal", lambda: test_bot_channel_ping(args.port, args.password)),
            ("Bot ignora NOTICE", lambda: test_bot_ignores_notice(args.port, args.password)),
            ("Nick Bot reservado", lambda: test_bot_nickname_is_reserved(args.port, args.password)),
            ("DCC SEND usuario a usuario", lambda: test_dcc_send_user_to_user(args.port, args.password)),
            ("DCC SEND fragmentado", lambda: test_dcc_send_fragmented(args.port, args.password)),
            ("DCC SEND nick inexistente", lambda: test_dcc_send_to_missing_user(args.port, args.password)),
            (
                "TOPIC exige pertenencia al canal",
                lambda: test_topic_requires_membership(args.port, args.password),
            ),
            (
                "INVITE rechaza usuarios ya presentes",
                lambda: test_invite_rejects_existing_member(args.port, args.password),
            ),
            (
                "MODE desconocido devuelve 472",
                lambda: test_unknown_mode_returns_472(args.port, args.password),
            ),
            (
                "TOPIC con +t exige operador",
                lambda: test_topic_restricted_requires_operator(
                    args.port, args.password
                ),
            ),
            (
                "Canal +i permite entrar solo con invitacion",
                lambda: test_invite_only_join_flow(args.port, args.password),
            ),
            (
                "MODE +itkl aplica key y limite",
                lambda: test_mode_key_and_limit_flow(args.port, args.password),
            ),
            (
                "Overflow de buffer desconecta al cliente",
                lambda: test_buffer_overflow_disconnect(args.port, args.password),
            ),
            (
                "Linea > 512 bytes no tumba el servidor",
                lambda: test_overlong_command_stability(args.port, args.password),
            ),
            (
                "Cierres abruptos en masa",
                lambda: test_mass_disconnect(args.port, args.password),
            ),
            (
                "Flood de mensajes mantiene liveness",
                lambda: test_message_flood_liveness(args.port, args.password),
            ),
        ]
        for name, callback in tests:
            run_test_case(name, callback, failures)
    finally:
        runner.stop()

    if args.use_valgrind:
        print("\n[INFO] Ejecutando pasada extra con el comando exacto del README...")
        vg_runner = ServerRunner(binary, args.port, args.password, True)
        try:
            vg_runner.start()
            run_valgrind_smoke(args.port, args.password)
        except Exception as exc:
            failures.append(("Valgrind", str(exc)))
            print("[FAIL] Valgrind -> " + str(exc))
        finally:
            vg_runner.stop()

        vg_summary = summarize_valgrind(vg_runner.read_valgrind_log())
        if vg_summary:
            print("\n[VALGRIND]")
            print(vg_summary)

    print("\nResumen:")
    print("  Fallos: {0}".format(len(failures)))
    print("  Log servidor: {0}".format(runner.server_log_path))
    if args.use_valgrind:
        print("  Log valgrind: {0}".format(os.path.join(ARTIFACTS_DIR, "valgrind.log")))

    if failures:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
