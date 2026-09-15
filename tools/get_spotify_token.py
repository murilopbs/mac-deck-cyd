#!/usr/bin/env python3
"""
MacDeck CYD - Spotify Token Helper
Autoriza sua conta Spotify e envia os tokens diretamente para o MacDeck CYD.

Uso:
  python3 tools/get_spotify_token.py
"""

import sys
import os
import json
import base64
import urllib.request
import urllib.parse
from http.server import HTTPServer, BaseHTTPRequestHandler
import webbrowser

PORT = 8888
REDIRECT_URI = f"http://127.0.0.1:{PORT}/callback"
SCOPES = "user-read-playback-state user-read-currently-playing user-modify-playback-state"

auth_code = None
server_instance = None

class OAuthCallbackHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        global auth_code
        url_parts = urllib.parse.urlparse(self.path)
        
        if url_parts.path == "/callback":
            params = urllib.parse.parse_qs(url_parts.query)
            if "code" in params:
                auth_code = params["code"][0]
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.end_headers()
                html = """
                <!DOCTYPE html>
                <html>
                <head>
                    <meta charset="utf-8">
                    <title>MacDeck CYD - Spotify</title>
                    <style>
                        body { background: #0d1117; color: #f0f6fc; font-family: -apple-system, sans-serif; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }
                        .card { background: #161b22; border: 1px solid #1DB954; border-radius: 16px; padding: 40px; text-align: center; max-width: 440px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
                        h1 { color: #1DB954; font-size: 1.6rem; margin-bottom: 12px; }
                        p { color: #8b949e; line-height: 1.5; font-size: 0.95rem; }
                        .badge { display: inline-block; background: #238636; color: #fff; padding: 6px 14px; border-radius: 20px; font-weight: bold; margin-top: 15px; }
                    </style>
                </head>
                <body>
                    <div class="card">
                        <h1>🎉 Autorização Concluída!</h1>
                        <p>O Spotify autorizou o acesso com sucesso. Você pode fechar esta aba e voltar para o terminal.</p>
                        <div class="badge">Configurando o MacDeck CYD...</div>
                    </div>
                </body>
                </html>
                """
                self.wfile.write(html.encode("utf-8"))
            elif "error" in params:
                error = params["error"][0]
                self.send_response(400)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.end_headers()
                self.wfile.write(f"<h2>Erro na autorização do Spotify: {error}</h2>".encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, format, *args):
        # Silencia logs padrão do servidor HTTP
        pass

def main():
    print("=" * 60)
    print("  🎛️  MacDeck CYD - Assistente de Autenticação Spotify")
    print("=" * 60)
    print("\nℹ️  No Spotify Developer Dashboard, certifique-se de que cadastrou:")
    print(f"   👉 Redirect URI: \033[92m{REDIRECT_URI}\033[0m\n")

    client_id = input("1. Digite ou cole seu Client ID: ").strip()
    if not client_id:
        print("❌ Client ID é obrigatório!")
        sys.exit(1)

    client_secret = input("2. Digite ou cole seu Client Secret: ").strip()
    if not client_secret:
        print("❌ Client Secret é obrigatório!")
        sys.exit(1)

    # Inicia servidor local para capturar o callback na porta 8888
    try:
        httpd = HTTPServer(("127.0.0.1", PORT), OAuthCallbackHandler)
    except OSError as e:
        print(f"❌ Não foi possível abrir a porta {PORT}. Verifique se já não há outro processo rodando nela.")
        sys.exit(1)

    # Monta a URL de autorização do Spotify
    auth_params = {
        "client_id": client_id,
        "response_type": "code",
        "redirect_uri": REDIRECT_URI,
        "scope": SCOPES
    }
    auth_url = "https://accounts.spotify.com/authorize?" + urllib.parse.urlencode(auth_params)

    print("\n🌐 Abrindo seu navegador para login no Spotify...")
    print(f"Se não abrir automaticamente, acesse:\n{auth_url}\n")
    webbrowser.open(auth_url)

    print("⏳ Aguardando você autorizar no navegador...")
    while auth_code is None:
        httpd.handle_request()

    httpd.server_close()
    print("✅ Código de autorização recebido!")

    # Troca o código temporário pelo refresh_token permanente
    print("🔄 Trocando código por tokens permanentes com a API do Spotify...")
    token_url = "https://accounts.spotify.com/api/token"
    token_data = urllib.parse.urlencode({
        "grant_type": "authorization_code",
        "code": auth_code,
        "redirect_uri": REDIRECT_URI
    }).encode("utf-8")

    creds = f"{client_id}:{client_secret}"
    basic_auth = base64.b64encode(creds.encode("utf-8")).decode("utf-8")

    req = urllib.request.Request(token_url, data=token_data, headers={
        "Authorization": f"Basic {basic_auth}",
        "Content-Type": "application/x-www-form-urlencoded"
    })

    try:
        with urllib.request.urlopen(req) as resp:
            resp_data = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        print(f"\n❌ Erro ao obter tokens do Spotify: HTTP {e.code}")
        print(e.read().decode("utf-8"))
        sys.exit(1)

    refresh_token = resp_data.get("refresh_token")
    access_token = resp_data.get("access_token")

    if not refresh_token:
        print("❌ Não foi retornado um refresh_token pelo Spotify.")
        sys.exit(1)

    print("\n" + "=" * 60)
    print("🎉 AUTORIZAÇÃO REALIZADA COM SUCESSO!")
    print("=" * 60)
    print(f"\n🔑 Seu Refresh Token permanente:\n\033[92m{refresh_token}\033[0m\n")

    # Pergunta ou tenta enviar diretamente ao MacDeck via mDNS ou IP
    macdeck_target = input("Deseja enviar automaticamente para o MacDeck? (Pressione Enter para 'http://macdeck.local' ou digite o IP): ").strip()
    if not macdeck_target:
        macdeck_target = "http://macdeck.local"
    elif not macdeck_target.startswith("http://") and not macdeck_target.startswith("https://"):
        macdeck_target = "http://" + macdeck_target

    post_url = macdeck_target.rstrip("/") + "/api/spotify"
    payload = json.dumps({
        "client_id": client_id,
        "client_secret": client_secret,
        "refresh_token": refresh_token
    }).encode("utf-8")

    print(f"\n📡 Enviando credenciais para {post_url}...")
    try:
        post_req = urllib.request.Request(post_url, data=payload, headers={
            "Content-Type": "application/json"
        })
        with urllib.request.urlopen(post_req, timeout=5) as p_resp:
            if p_resp.status == 200:
                print("✨ \033[92mMacDeck CYD configurado e pronto! O player já está ativo na tela!\033[0m")
            else:
                print(f"⚠️ Resposta do MacDeck: HTTP {p_resp.status}")
    except Exception as e:
        print(f"⚠️ Não foi possível conectar ao MacDeck automaticamente ({e}).")
        print("Não tem problema! Basta abrir o painel web do MacDeck e colar o Refresh Token acima no campo 'Refresh Token'!")

    print("\n✅ Concluído com sucesso!\n")

if __name__ == "__main__":
    main()
