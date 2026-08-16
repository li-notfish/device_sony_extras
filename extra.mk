#
# Copyright (C) 2024 XperiaLabs Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Soong Namespace
PRODUCT_SOONG_NAMESPACES += \
    vendor/sony/extras

PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,device/sony/extras/lib/common/framework/system/,$(TARGET_COPY_OUT_SYSTEM)/) \
    $(call find-copy-subdir-files,*,device/sony/extras/lib/common/framework/product/,$(TARGET_COPY_OUT_PRODUCT)/)

# LineageOS overrides
TARGET_EXCLUDES_AUDIOFX := true
PRODUCT_NO_CAMERA := true

# Audio Enhancements
$(call inherit-product, device/sony/extras/lib/yodo/audio/audio.mk)

# Vendor files
$(call inherit-product, vendor/sony/extras/extras-vendor.mk)
