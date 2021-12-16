
node("windows-1") {
  stage("Build CASM for Windows") {
    dir("build"){
      bat 'cmake .. -G "Unix Makefiles"'
      bat 'make'
      archiveArtifacts artifacts: 'casm.exe', fingerprint: true
      deleteDir()
    }
  }
}
node("master") {
  stage("Build CASM-STATIC for Linux") {
    dir("build"){
      sh 'cmake .. -DUSE_GIT_VERSION=true -DBUILD_STATIC=true'
      sh 'make'
      archiveArtifacts artifacts: 'casm-static*', fingerprint: true
      deleteDir()
    }
  }
}
