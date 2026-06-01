#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_lib import lib_fixups

from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

from extract_utils.fixups_blob import (
    blob_fixups_user_type,
    blob_fixup,
)
from extract_utils.module import lib_fixups_user_type

namespace_imports = [
    'hardware/qcom-caf/sm8550',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/sony/sm8550-common',
]

blob_fixups: blob_fixups_user_type = {
    (
        'vendor/lib/libiVptLibC.so',
        'vendor/lib/libHpEqApi.so',
        'vendor/lib64/libiVptLibC.so',
        'vendor/lib64/libHpEqApi.so',
    ): blob_fixup()
        .add_needed('libcrypto.so')
        .add_needed('libiVptHkiDec.so'),
    (
    'vendor/lib/libiVptApi.so',
    'vendor/lib64/libiVptApi.so',
    ): blob_fixup()
        .add_needed('libiVptLibC.so'),
    'vendor/bin/hw/vendor.dolby.media.c2@1.0-service': blob_fixup()
        .add_needed("libshim_dolby.so"),
    (
     'vendor/etc/init/vendor.dolby.media.c2@1.0-service.rc',
     'vendor/etc/init/android.hardware.media.c2@1.0-threesixty-ra-service.rc',
    ): blob_fixup()
	.regex_replace('writepid.*', 'task_profiles ProcessCapacityHigh HighPerformance'),
    'vendor/lib64/soundfx/libswdap.so': blob_fixup()
    .binary_regex_replace(b'\x1f\x00\x00\x71\xe0\x03\x00\x91\xf3\x17\x9f\x1a\x41\x62\x04\x94',
                          b'\x1f\x00\x00\x71\xe0\x03\x00\x91\x13\x00\x80\x52\x41\x62\x04\x94')
    .binary_regex_replace(rb'\x09\x00\x00\x12\x89\x02\x09\x0b\x3f\x01\x08\x6b\xca\x01\x00\x54',
                          b'\x09\x00\x00\x12\x89\x02\x09\x0b\x3f\x01\x08\x6b\x0e\x00\x00\x14')
    'vendor/etc/dolby/dax-default.xml': blob_fixup()
        .regex_replace('volume-leveler-enable value="true"', 'volume-leveler-enable value="false"'),
}

module = ExtractUtilsModule(
    'extras',
    'sony',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
