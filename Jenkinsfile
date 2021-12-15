pipeline {
  agent { label "windows-1" }
  stages {
    stage('Prepare') {
      steps {
        sh 'mkdir build'
        sh 'cd build && cmake -DBoost_LIBRARY_DIR=C:/SDKs/boost_1_76_0/lib -DBoost_INCLUDE_DIR=C:/SDKs/boost_1_76_0 .. -G "Unix Makefiles"'
      }
    }

    stage('Build') {
      steps {
        sh 'cd build && make'
      }
    }

  }
}
