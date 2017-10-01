/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * libcadence Unit Test Suit
 * @file: GTest entry point
 * @author: soultaker
 *
 *******************************************************************************/
#include "gtest/gtest.h"
#include "ci/teamcity_gtest.h"


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    if (jetbrains::teamcity::underTeamcity()) {
        auto& listeners = testing::UnitTest::GetInstance()->listeners();

        // Add unique flowId parameter if you want to run test processes in parallel
        // See http://confluence.jetbrains.net/display/TCD6/Build+Script+Interaction+with+TeamCity#BuildScriptInteractionwithTeamCity-MessageFlowId
        listeners.Append(new jetbrains::teamcity::TeamcityGoogleTestEventListener());
    }

    return RUN_ALL_TESTS();
}
