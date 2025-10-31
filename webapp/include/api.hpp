#pragma once
#include <crow.h>
#include "repo.hpp"

// Registers all routes on the given app.
void register_routes(crow::SimpleApp& app, Repo& repo);
