load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# ══════════════════════════════════════════════════════════════════════
# FAÇADE LIBRARY (Phase 6)
# ══════════════════════════════════════════════════════════════════════

cc_library(
    name = "facade",
    srcs = [
        "AppConfig.cpp",
        "TelemetryApp.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//inc/Facade:facade_hdrs",
        "//src:async_logging",
        "//src:smart_data_hub",
        "//src:logging",
    ],
)
