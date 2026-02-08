load("@rules_cc//cc:defs.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

# ══════════════════════════════════════════════════════════════════════
# TELEMETRY SERVICE (Server)
# ══════════════════════════════════════════════════════════════════════

cc_binary(
    name = "telemetry_service",
    srcs = ["telemetry_service.cpp"],
    copts = ["-I/usr/local/include"],
    linkopts = [
        "-L/usr/local/lib",
        "-lvsomeip3",
        "-lvsomeip3-cfg",
        "-lpthread",
    ],
)

# ══════════════════════════════════════════════════════════════════════
# TELEMETRY CLIENT (Test client using SomeIPTelemetrySourceImpl)
# ══════════════════════════════════════════════════════════════════════

cc_binary(
    name = "telemetry_client",
    srcs = ["telemetry_client.cpp"],
    copts = ["-I/usr/local/include"],
    deps = [
        "//src:vsomeip_telemetry",
    ],
)

# ══════════════════════════════════════════════════════════════════════
# MAIN (Integration with AsyncLogManager)
# ══════════════════════════════════════════════════════════════════════

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    copts = ["-I/usr/local/include"],
    deps = [
        "//src:async_logging",
        "//src:logging",
        "//src:vsomeip_telemetry",
    ],
)

