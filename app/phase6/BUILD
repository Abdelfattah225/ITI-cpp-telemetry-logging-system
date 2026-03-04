load("@rules_cc//cc:defs.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    data = ["config.json"],  # Make config available at runtime
    deps = [
        "//src/Facade:facade",
    ],
)
