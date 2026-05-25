#include "http/router.hpp"
#include <cassert>
#include <iostream>

using https_server::Router;
using https_server::http::HttpRequest;
using https_server::http::HttpResponse;

int main() {
    Router router;

    router.use([](const HttpRequest& request, https_server::NextHandler next) {
        auto response = next(request);
        response.headers["X-Test-Middleware"] = request.path;
        return response;
    });

    router.add_route("GET", "/users/me", [](const HttpRequest&) {
        HttpResponse response;
        response.body = "me";
        return response;
    });

    router.add_route("GET", "/users/:id", [](const HttpRequest& request) {
        HttpResponse response;
        response.body = request.path_params.at("id");
        return response;
    });

    router.add_route("POST", "/users/:id", [](const HttpRequest&) {
        HttpResponse response;
        response.status_code = 204;
        response.status_text = "No Content";
        return response;
    });

    HttpRequest exact;
    exact.method = "GET";
    exact.path = "/users/me";
    auto exact_response = router.route_request(exact);
    assert(exact_response.body == "me");
    assert(exact_response.headers.at("X-Test-Middleware") == "/users/me");

    HttpRequest param;
    param.method = "GET";
    param.path = "/users/123";
    auto param_response = router.route_request(param);
    assert(param_response.body == "123");

    HttpRequest missing_method;
    missing_method.method = "DELETE";
    missing_method.path = "/users/123";
    auto method_response = router.route_request(missing_method);
    assert(method_response.status_code == 405);

    HttpRequest missing_path;
    missing_path.method = "GET";
    missing_path.path = "/missing";
    auto missing_response = router.route_request(missing_path);
    assert(missing_response.status_code == 404);

    std::cout << "test_router passed\n";
    return 0;
}
