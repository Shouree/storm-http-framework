import sys
import pytest
import subprocess
import socket

SERVER_HOST = "localhost"
SERVER_PORT = 1234
BASE_URL = f"http://{SERVER_HOST}:{SERVER_PORT}"

def run_curl_request(method, url, headers=None, data=None):
    curl_command = ["curl", "-iX", method, url]
    
    if headers:
        for header, value in headers.items():
            curl_command += ["-H", f"{header}: {value}"]

    if data:
        curl_command += ["--data", data]

    result = subprocess.run(curl_command, capture_output=True, text=True)
    return result

def test_get_request():
    result = run_curl_request("GET", BASE_URL)
    assert result.returncode == 0
    assert "HTTP/1.1 200 OK" in result.stdout

def test_post_request():
    data = "name=Stormy_Daniels&age=30"
    headers = {"Content-Type": "application/x-www-form-urlencoded"}
    result = run_curl_request("POST", BASE_URL, headers=headers, data=data)
    assert result.returncode == 0
    assert "HTTP/1.1 200 OK" in result.stdout

def test_put_request():
    data = '{"id":1,"name":"Test"}'
    headers = {"Content-Type": "application/json"}
    result = run_curl_request("PUT", BASE_URL, headers=headers, data=data)
    assert result.returncode == 0
    assert "HTTP/1.1 200 OK" in result.stdout
def teset_relative_url():
    result = run_curl_request("GET", BASE_URL + "/chat")
    assert result.returncode == 0
    assert "HTTP/1.1 200 OK" in result.stdout

def test_weird_requests():
    request = (
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
            "X-Custom-Header: Weird:Value\r\n"
        "\r\n"
    )

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode())
        response = s.recv(4096).decode()

    assert "HTTP/1.1 200 OK" in response

    """   request = (
        "GET / HTTP/1.1\n"
        "Host: localhost\n"
            "X-Custom-Header: No backslash r\n" #< saknar \r 
        "\r\n"
    )

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode())
        response = s.recv(4096).decode()

    assert "400 Bad Request" in response

    request = (
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: jättemycket\r\n" #fel Content-Length
        "\r\n"
        "Ganska mycket"
    )

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode())
        response = s.recv(4096).decode()

    assert "400 Bad Request" in response
    """
    """request = "GIBBERISH / HTTP/1.1\r\nHost: localhost\r\n\r\n" #<GIBBERISH 

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode())
        response = s.recv(4096).decode()

    assert "400 Bad Request" in response"""

    request = "GET /etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n" #<icke giltig sökväg 

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode())
        response = s.recv(4096).decode()

    assert "404 Not Found" in response #< Eller ska detta också vara Bad Request?

def test_long_inputs():
    long_uri = "/EnMycketStorURI" * 1024*1024*1024

    request = (
        f"GET {long_uri} HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    )

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_HOST, SERVER_PORT))
        s.sendall(request.encode())
        response = s.recv(4096).decode()

    assert "414 Request-URI Too Long" in response

    data = "{" + '"id":1,"name":"Test",'*1024*1024*1024 +"}" #< borde bli minst 1 MiB+
    headers = {"Content-Type": "application/json"}
    result = run_curl_request("PUT", BASE_URL, headers=headers, data=data)
    assert result.returncode == 0
    assert "413 Request Entity Too Large" in result.stdout



if __name__ == "__main__":
    pytest.main()

