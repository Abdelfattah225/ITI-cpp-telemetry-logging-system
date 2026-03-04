load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "facade_hdrs",
    hdrs = [
        "AppConfig.hpp",
        "TelemetryApp.hpp",
        "json.hpp",
    ],
    include_prefix = "",
    strip_include_prefix = "",
    includes = ["."],  # Adds inc/Facade to include path
)
