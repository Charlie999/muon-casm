pipeline {
  agent any
  node("windows-1") {
    stages {
      stage('Build') {
        steps {
          bat 'mkdir build'
          bat 'cd build && cmake .. -G "Unix Makefiles"'
          bat 'cd build && make'
        }
      }
    }
  }
  node("master") {
    stages {
      stage('Build') {
        steps {
          sh 'mkdir build'
          sh 'cd build && cmake .. -DUSE_GIT_VERSION=true -DBUILD_STATIC=true'
          sh 'cd build && make'
        }
      }
    }
  }
  
  post {
      always {
          archiveArtifacts artifacts: 'build/casm.exe, build/casm-static*', fingerprint: true
      }
  }
}
