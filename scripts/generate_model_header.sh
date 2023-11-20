#!/bin/bash

# Model.h file path
HEADER_FILE=$1

# JETSON_MODELS values
JETSON_MODELS="${@:2}"

# Add header file generation command
cat << EOF > $HEADER_FILE
/*
   This is an automatically generated file.
   Do not manually modify this file.
   Any changes made here may be overwritten.
*/

#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <string>

namespace GPIO 
{
    enum class Model 
    {
EOF

# Add enum class items using a for loop
for MODEL in $JETSON_MODELS; do
  echo "        $MODEL," >> $HEADER_FILE
done

# Add remaining part
cat << EOF >> $HEADER_FILE
    };

    // names
    constexpr const char* MODEL_NAMES[] = {
EOF

# Add enum names using a for loop
for MODEL in $JETSON_MODELS; do
  echo "        \"$MODEL\"," >> $HEADER_FILE
done

# Add remaining part
cat << EOF >> $HEADER_FILE
    };

    // alias
EOF

# Add enum alias generation commands
for MODEL in $JETSON_MODELS; do
  echo "    constexpr Model $MODEL = Model::$MODEL;" >> $HEADER_FILE
done

# Add remaining part
cat << EOF >> $HEADER_FILE
} // namespace GPIO

#endif
EOF
