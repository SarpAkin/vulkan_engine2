#!/bin/bash

find res/ \( -name "*.frag" -o -name "*.vert" -o -name "*.comp" \) -exec glslc {} -o "{}.spv" \;
mv res/*.spv res/spv/