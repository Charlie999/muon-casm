pipeline {
  agent { label "windows-1" }
  stages {
    stage('Build') {
      steps {
        bat 'mkdir build'
        bat 'cd build && cmake .. -G "Unix Makefiles"'
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
