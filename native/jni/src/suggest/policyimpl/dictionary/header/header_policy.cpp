/*
 * Copyright (C) 2013, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "suggest/policyimpl/dictionary/header/header_policy.h"

namespace latinime {

// Note that these are corresponding definitions in Java side in FormatSpec.FileHeader.
const char *const HeaderPolicy::MULTIPLE_WORDS_DEMOTION_RATE_KEY = "MULTIPLE_WORDS_DEMOTION_RATE";
const char *const HeaderPolicy::REQUIRES_GERMAN_UMLAUT_PROCESSING_KEY =
        "REQUIRES_GERMAN_UMLAUT_PROCESSING";
// TODO: Change attribute string to "IS_DECAYING_DICT".
const char *const HeaderPolicy::IS_DECAYING_DICT_KEY = "USES_FORGETTING_CURVE";
const char *const HeaderPolicy::DATE_KEY = "date";
const char *const HeaderPolicy::LAST_DECAYED_TIME_KEY = "LAST_DECAYED_TIME";
const char *const HeaderPolicy::UNIGRAM_COUNT_KEY = "UNIGRAM_COUNT";
const char *const HeaderPolicy::BIGRAM_COUNT_KEY = "BIGRAM_COUNT";
const char *const HeaderPolicy::EXTENDED_REGION_SIZE_KEY = "EXTENDED_REGION_SIZE";
// Historical info is information that is needed to support decaying such as timestamp, level and
// count.
const char *const HeaderPolicy::HAS_HISTORICAL_INFO_KEY = "HAS_HISTORICAL_INFO";
const char *const HeaderPolicy::LOCALE_KEY = "locale"; // match Java declaration
const int HeaderPolicy::DEFAULT_MULTIPLE_WORDS_DEMOTION_RATE = 100;
const float HeaderPolicy::MULTIPLE_WORD_COST_MULTIPLIER_SCALE = 100.0f;

// Used for logging. Question mark is used to indicate that the key is not found.
void HeaderPolicy::readHeaderValueOrQuestionMark(const char *const key, int *outValue,
        int outValueSize) const {
    if (outValueSize <= 0) return;
    if (outValueSize == 1) {
        outValue[0] = '\0';
        return;
    }
    std::vector<int> keyCodePointVector;
    HeaderReadWriteUtils::insertCharactersIntoVector(key, &keyCodePointVector);
    HeaderReadWriteUtils::AttributeMap::const_iterator it = mAttributeMap.find(keyCodePointVector);
    if (it == mAttributeMap.end()) {
        // The key was not found.
        outValue[0] = '?';
        outValue[1] = '\0';
        return;
    }
    const int terminalIndex = min(static_cast<int>(it->second.size()), outValueSize - 1);
    for (int i = 0; i < terminalIndex; ++i) {
        outValue[i] = it->second[i];
    }
    outValue[terminalIndex] = '\0';
}

const std::vector<int> HeaderPolicy::readLocale() const {
    return HeaderReadWriteUtils::readCodePointVectorAttributeValue(&mAttributeMap, LOCALE_KEY);
}

float HeaderPolicy::readMultipleWordCostMultiplier() const {
    const int demotionRate = HeaderReadWriteUtils::readIntAttributeValue(&mAttributeMap,
            MULTIPLE_WORDS_DEMOTION_RATE_KEY, DEFAULT_MULTIPLE_WORDS_DEMOTION_RATE);
    if (demotionRate <= 0) {
        return static_cast<float>(MAX_VALUE_FOR_WEIGHTING);
    }
    return MULTIPLE_WORD_COST_MULTIPLIER_SCALE / static_cast<float>(demotionRate);
}

bool HeaderPolicy::readRequiresGermanUmlautProcessing() const {
    return HeaderReadWriteUtils::readBoolAttributeValue(&mAttributeMap,
            REQUIRES_GERMAN_UMLAUT_PROCESSING_KEY, false);
}

bool HeaderPolicy::fillInAndWriteHeaderToBuffer(const bool updatesLastDecayedTime,
        const int unigramCount, const int bigramCount,
        const int extendedRegionSize, BufferWithExtendableBuffer *const outBuffer) const {
    int writingPos = 0;
    HeaderReadWriteUtils::AttributeMap attributeMapToWrite(mAttributeMap);
    fillInHeader(updatesLastDecayedTime, unigramCount, bigramCount,
            extendedRegionSize, &attributeMapToWrite);
    if (!HeaderReadWriteUtils::writeDictionaryVersion(outBuffer, mDictFormatVersion,
            &writingPos)) {
        return false;
    }
    if (!HeaderReadWriteUtils::writeDictionaryFlags(outBuffer, mDictionaryFlags,
            &writingPos)) {
        return false;
    }
    // Temporarily writes a dummy header size.
    int headerSizeFieldPos = writingPos;
    if (!HeaderReadWriteUtils::writeDictionaryHeaderSize(outBuffer, 0 /* size */,
            &writingPos)) {
        return false;
    }
    if (!HeaderReadWriteUtils::writeHeaderAttributes(outBuffer, &attributeMapToWrite,
            &writingPos)) {
        return false;
    }
    // Writes the actual header size.
    if (!HeaderReadWriteUtils::writeDictionaryHeaderSize(outBuffer, writingPos,
            &headerSizeFieldPos)) {
        return false;
    }
    return true;
}

void HeaderPolicy::fillInHeader(const bool updatesLastDecayedTime, const int unigramCount,
        const int bigramCount, const int extendedRegionSize,
        HeaderReadWriteUtils::AttributeMap *outAttributeMap) const {
    HeaderReadWriteUtils::setIntAttribute(outAttributeMap, UNIGRAM_COUNT_KEY, unigramCount);
    HeaderReadWriteUtils::setIntAttribute(outAttributeMap, BIGRAM_COUNT_KEY, bigramCount);
    HeaderReadWriteUtils::setIntAttribute(outAttributeMap, EXTENDED_REGION_SIZE_KEY,
            extendedRegionSize);
    // Set the current time as the generation time.
    HeaderReadWriteUtils::setIntAttribute(outAttributeMap, DATE_KEY,
            TimeKeeper::peekCurrentTime());
    HeaderReadWriteUtils::setCodePointVectorAttribute(outAttributeMap, LOCALE_KEY, mLocale);
    if (updatesLastDecayedTime) {
        // Set current time as the last updated time.
        HeaderReadWriteUtils::setIntAttribute(outAttributeMap, LAST_DECAYED_TIME_KEY,
                TimeKeeper::peekCurrentTime());
    }
}

/* static */ HeaderReadWriteUtils::AttributeMap
        HeaderPolicy::createAttributeMapAndReadAllAttributes(const uint8_t *const dictBuf) {
    HeaderReadWriteUtils::AttributeMap attributeMap;
    HeaderReadWriteUtils::fetchAllHeaderAttributes(dictBuf, &attributeMap);
    return attributeMap;
}

} // namespace latinime
