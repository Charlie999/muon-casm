pipeline {
  agent { label "windows-1" }
  stages {
    stage('Build') {
      steps {
        bat 'mkdir build'
        bat 'cd build && cmake -DBoost_LIBRARY_DIR=C:/SDKs/boost_1_78_0/lib64-msvc-14.2 -DBoost_INCLUDE_DIR=C:/SDKs/boost_1_78_0 -DBoost_ROOT=C:/SDKs/boost_1_78_0/boost .. -G "Unix Makefiles"'
        bat 'cd build && make'
      }
    }
  }
  
  post {
      always {
          archiveArtifacts artifacts: 'build/casm.exe', fingerprint: true
      }
  }
}
