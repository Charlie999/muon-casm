pipeline {
  agent { label "windows-1" }
  stages {
    stage('Prepare') {
      steps {
        sh 'mkdir build'
        sh 'cd build && cmake ..'
      }
    }

    stage('Build') {
      steps {
        sh 'cd build && make'
      }
    }

  }
}
