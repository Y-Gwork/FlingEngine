﻿node {

    sh "echo \$PWD"

    stage('Preparation') { 
        // TODO: Update this to actually be correct ? 
        // TODO: Just use the docker builder plugin here
        // Get some code from a GitHub repository
        git 'https://github.com/flingengine/FlingEngine.git'
        sh "./Init.sh"
    }
}
