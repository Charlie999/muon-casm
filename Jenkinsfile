pipeline {
  agent { label "windows-1" }
  stages {
    stage('Build') {
      steps {
        bat 'mkdir build'
        bat 'cd build && cmake -DBoost_LIBRARY_DIR=C:/SDKs/boost_1_76_0/lib -DBoost_INCLUDE_DIR=C:/SDKs/boost_1_76_0 .. -G "Unix Makefiles"'
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
